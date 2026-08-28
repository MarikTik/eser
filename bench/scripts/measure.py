#!/usr/bin/env python3
"""Static-footprint and compile-time benchmark driver for eser.

Builds every ``<board>_t<tier>`` environment in ``bench/platformio.ini``, reads the true section
sizes out of each firmware ELF with the toolchain's own ``size`` tool, and emits a per-board table
of absolute footprint plus the incremental cost of each codec feature tier.

Compile time is measured as wall-clock of a *clean* build (``pio run -t clean`` first), because an
incremental rebuild mostly measures the framework archive that every tier shares.

Usage::

    python3 bench/scripts/measure.py                  # all boards, all tiers
    python3 bench/scripts/measure.py --boards esp32dev nodemcuv2
    python3 bench/scripts/measure.py --json out.json  # also dump raw records

Builds run one at a time, deliberately. Building several environments concurrently exhausts memory
on an ordinary workstation -- each Xtensa/ARM compile holds a large working set, and a handful in
parallel is enough to invoke the OOM killer -- and it also destroys the compile-time column, whose
wall-clock would then measure contention rather than the code. The sequential run is roughly five
seconds per environment once the frameworks are cached.

A board whose toolchain cannot build eser at all (an open question for AVR, whose libstdc++ may
lack ``std::optional``) is recorded as unsupported with the compiler's first error, and does not
abort the rest of the run.
"""

from __future__ import annotations

import argparse
import configparser
import json
import re
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass, asdict
from pathlib import Path

BENCH_DIR = Path(__file__).resolve().parent.parent
INI = BENCH_DIR / "platformio.ini"

MODE_LABELS = {
    "rel": "-Os -DNDEBUG, ships",
    "relO2": "-O2 -DNDEBUG",
    "dbg": "-Og, asserts live",
}

# Presentation order for the cross-mode summary table.
MODES_ORDER = [("rel", 0), ("relO2", 1), ("dbg", 2)]

TIER_LABELS = {
    0: "framework only (no eser)",
    1: "+ include eser.hpp",
    2: "+ scalar codec (native LE)",
    3: "+ array codec",
    4: "+ big-endian wire",
    5: "+ fixed_string",
    6: "+ struct (raw image)",
}


@dataclass
class Record:
    board: str
    mode: str
    tier: int
    ok: bool
    text: int = 0
    rodata: int = 0
    data: int = 0
    bss: int = 0
    flash: int = 0
    ram: int = 0
    build_seconds: float = 0.0
    error: str = ""
    flag_warning: str = ""


def environments() -> dict[tuple[str, str], list[int]]:
    """Map (board, mode) -> sorted tiers, parsed from platformio.ini env sections."""
    cp = configparser.ConfigParser()
    cp.read(INI)
    groups: dict[tuple[str, str], list[int]] = {}
    for section in cp.sections():
        if not section.startswith("env:"):
            continue
        name = section[len("env:"):]
        m = re.fullmatch(r"(.+)_([A-Za-z0-9]+)_t(\d+)", name)
        if not m:
            continue
        groups.setdefault((m.group(1), m.group(2)), []).append(int(m.group(3)))
    for tiers in groups.values():
        tiers.sort()
    return groups


def elf_machine(elf: Path) -> str:
    """Read e_machine from the ELF header, so the matching cross ``size`` tool can be chosen.

    Using the host ``size`` on a cross ELF, or an Xtensa tool on an ARM image, silently reports
    nothing or garbage -- which would look like a real footprint change rather than a broken read.
    """
    try:
        header = elf.read_bytes()[:20]
    except OSError:
        return ""
    if len(header) < 20 or header[:4] != b"\x7fELF":
        return ""
    little = header[5] == 1
    machine = int.from_bytes(header[18:20], "little" if little else "big")
    return {0x28: "arm", 0x53: "avr", 0x5E: "xtensa", 0xF3: "riscv"}.get(machine, "")


def size_tool_for(elf: Path) -> str | None:
    """Locate the cross ``size`` binary matching this ELF's architecture, else fall back to host."""
    patterns = {
        "xtensa": "xtensa-*-elf-size",
        "arm": "arm-none-eabi-size",
        "avr": "avr-size",
        "riscv": "riscv*-esp-elf-size",
    }
    pattern = patterns.get(elf_machine(elf))
    if pattern:
        for path in sorted(Path.home().joinpath(".platformio/packages").glob(f"*/bin/{pattern}")):
            return str(path)
    return shutil.which("size")


def read_sections(elf: Path) -> dict[str, int]:
    """Sum ELF section sizes by name using ``size -A``.

    Sections are bucketed rather than taken verbatim: ESP-IDF splits code and constants across
    many named sections (``.iram0.text``, ``.flash.rodata``, ...) that must be grouped to be
    comparable with a flat AVR or STM32 layout.
    """
    tool = size_tool_for(elf)
    if not tool:
        return {}
    try:
        out = subprocess.run([tool, "-A", str(elf)], capture_output=True, text=True, timeout=60).stdout
    except (subprocess.SubprocessError, OSError):
        return {}

    buckets = {"text": 0, "rodata": 0, "data": 0, "bss": 0}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) < 2:
            continue
        name, raw = parts[0], parts[1]
        if not name.startswith(".") or not raw.isdigit():
            continue
        n, size = name.lower(), int(raw)
        if "bss" in n or "noinit" in n:
            buckets["bss"] += size
        elif "rodata" in n:
            buckets["rodata"] += size
        elif "text" in n or "vectors" in n or "iram" in n:
            buckets["text"] += size
        elif n.endswith(".data") or ".data." in n or "dram0.data" in n:
            buckets["data"] += size
    return buckets


def verify_flags(env: str, expect_opt: str, expect_ndebug: bool) -> str:
    """Confirm the intended -O level and NDEBUG actually reached the compile line.

    PlatformIO applies ``build_unflags`` after ``build_flags``, so an over-broad unflag list
    silently strips the very optimization level a mode is trying to set, leaving the tier at -O0
    and inflating its measured cost roughly tenfold. That failure is invisible in the size table
    -- the numbers look plausible, just wrong -- so it is checked rather than assumed.
    """
    # The object must be removed first: a verbose run over an up-to-date build prints no compile
    # line at all, which would look like a missing flag rather than a skipped recompile.
    obj = BENCH_DIR / ".pio" / "build" / env / "src" / "main.cpp.o"
    try:
        obj.unlink(missing_ok=True)
        proc = subprocess.run(["pio", "run", "-e", env, "-v"],
                              cwd=BENCH_DIR, capture_output=True, text=True, timeout=1800)
    except (subprocess.SubprocessError, OSError) as exc:
        return f"flag check could not run: {exc}"

    line = next((l for l in proc.stdout.splitlines()
                 if "src/main.cpp" in l and l.strip().endswith("src/main.cpp")), "")
    if not line:
        return "flag check: no compile line for src/main.cpp found"

    tokens = line.split()
    opts = [t for t in tokens if re.fullmatch(r"-O[0-9sgz]?", t)]
    problems = []
    if expect_opt not in opts:
        problems.append(f"expected {expect_opt}, saw {opts or ['(none)']}")
    if expect_ndebug and "-DNDEBUG" not in tokens:
        problems.append("expected -DNDEBUG, absent")
    if not expect_ndebug and "-DNDEBUG" in tokens:
        problems.append("-DNDEBUG present in a debug build; asserts are compiled out")
    return "; ".join(problems)


def build(env: str, clean: bool = True) -> tuple[bool, float, str]:
    """Clean-build one environment; return (ok, wall_seconds, first_error_line)."""
    if clean:
        subprocess.run(["pio", "run", "-e", env, "-t", "clean"],
                       cwd=BENCH_DIR, capture_output=True, text=True, timeout=300)
    start = time.monotonic()
    proc = subprocess.run(["pio", "run", "-e", env],
                          cwd=BENCH_DIR, capture_output=True, text=True, timeout=1800)
    elapsed = time.monotonic() - start

    if proc.returncode == 0:
        return True, elapsed, ""

    combined = proc.stdout + proc.stderr
    for line in combined.splitlines():
        if "error:" in line.lower():
            return False, elapsed, line.strip()[:300]
    return False, elapsed, (combined.strip().splitlines() or ["build failed"])[-1][:300]


MODE_EXPECT = {
    "rel": ("-Os", True),
    "relO2": ("-O2", True),
    "dbg": ("-Og", False),
}


def measure(board: str, mode: str, tier: int, check_flags: bool = True) -> Record:
    env = f"{board}_{mode}_t{tier}"
    ok, seconds, err = build(env)
    if not ok:
        return Record(board=board, mode=mode, tier=tier, ok=False,
                      build_seconds=seconds, error=err)

    warning = ""
    if check_flags and mode in MODE_EXPECT:
        expect_opt, expect_ndebug = MODE_EXPECT[mode]
        warning = verify_flags(env, expect_opt, expect_ndebug)

    elf = BENCH_DIR / ".pio" / "build" / env / "firmware.elf"
    s = read_sections(elf) if elf.exists() else {}
    text, rodata = s.get("text", 0), s.get("rodata", 0)
    data, bss = s.get("data", 0), s.get("bss", 0)
    return Record(
        board=board, mode=mode, tier=tier, ok=True,
        text=text, rodata=rodata, data=data, bss=bss,
        flash=text + rodata + data, ram=data + bss,
        build_seconds=seconds, flag_warning=warning,
    )


def render(records: list[Record]) -> str:
    groups: dict[tuple[str, str], list[Record]] = {}
    for r in records:
        groups.setdefault((r.board, r.mode), []).append(r)

    out: list[str] = []
    totals: dict[tuple[str, str], tuple[int, int]] = {}

    for (board, mode), rows in groups.items():
        rows.sort(key=lambda r: r.tier)
        out.append(f"\n## {board}  [{mode}: {MODE_LABELS.get(mode, mode)}]\n")

        warned = {r.flag_warning for r in rows if r.flag_warning}
        for w in sorted(warned):
            out.append(f"  !! BUILD FLAGS: {w}")
            out.append("  !! Numbers below do not reflect the intended build mode.")
        if warned:
            out.append("")

        if not any(r.ok for r in rows):
            out.append("  UNSUPPORTED - no tier built.")
            first = next((r for r in rows if r.error), None)
            if first:
                out.append(f"  {first.error}")
            continue

        out.append(f"  {'tier':<28} {'flash':>8} {'Dflash':>8} {'ram':>7} {'Dram':>7} {'build':>8}")
        out.append(f"  {'-' * 28} {'-' * 8} {'-' * 8} {'-' * 7} {'-' * 7} {'-' * 8}")

        prev: Record | None = None
        for r in rows:
            label = TIER_LABELS.get(r.tier, f"tier {r.tier}")
            if not r.ok:
                out.append(f"  {label:<28} {'FAILED':>8}   {r.error[:40]}")
                continue
            d_flash = f"{r.flash - prev.flash:+d}" if prev and prev.ok else "-"
            d_ram = f"{r.ram - prev.ram:+d}" if prev and prev.ok else "-"
            out.append(
                f"  {label:<28} {r.flash:>8} {d_flash:>8} {r.ram:>7} {d_ram:>7}"
                f" {r.build_seconds:>7.1f}s"
            )
            prev = r

        built = [r for r in rows if r.ok]
        if len(built) >= 2:
            base, top = built[0], built[-1]
            d_flash, d_ram = top.flash - base.flash, top.ram - base.ram
            totals[(board, mode)] = (d_flash, d_ram)
            out.append("")
            out.append(f"  eser total cost (tier {base.tier} -> {top.tier}): "
                       f"{d_flash:+d} B flash, {d_ram:+d} B RAM")

    # Cross-mode summary: the assert-contract cost is only visible by comparing modes.
    if totals:
        out.append("\n")
        out.append("## eser total flash cost by build mode\n")
        boards = sorted({b for b, _ in totals})
        modes = [m for m, _ in MODES_ORDER if any((b, m) in totals for b in boards)]
        out.append(f"  {'board':<14}" + "".join(f"{m:>12}" for m in modes))
        out.append(f"  {'-' * 14}" + "".join(f"{'-' * 12}" for _ in modes))
        for b in boards:
            cells = "".join(
                f"{(str(totals[(b, m)][0]) + ' B') if (b, m) in totals else '-':>12}"
                for m in modes
            )
            out.append(f"  {b:<14}{cells}")
        out.append("")
        out.append("  A larger figure under 'dbg' is eser's assert-based contracts, which exist")
        out.append("  only while NDEBUG is undefined and vanish in the shipping builds.")

    return "\n".join(out)


def main() -> int:
    ap = argparse.ArgumentParser(description="eser static footprint benchmark")
    ap.add_argument("--boards", nargs="*", help="subset of boards to measure (default: all)")
    ap.add_argument("--modes", nargs="*", help="subset of build modes, e.g. rel dbg (default: all)")
    ap.add_argument("--json", type=Path, help="write raw records here")
    ap.add_argument("--no-flag-check", action="store_true",
                    help="skip verifying that -O/-DNDEBUG reached the compile line (faster)")
    args = ap.parse_args()

    if not shutil.which("pio"):
        print("error: PlatformIO CLI ('pio') not found on PATH", file=sys.stderr)
        return 1

    groups = environments()
    known_boards = sorted({b for b, _ in groups})
    known_modes = sorted({m for _, m in groups})

    if args.boards:
        unknown = set(args.boards) - set(known_boards)
        if unknown:
            print(f"error: unknown board(s): {', '.join(sorted(unknown))}", file=sys.stderr)
            print(f"known: {', '.join(known_boards)}", file=sys.stderr)
            return 1
        groups = {k: v for k, v in groups.items() if k[0] in args.boards}

    if args.modes:
        unknown = set(args.modes) - set(known_modes)
        if unknown:
            print(f"error: unknown mode(s): {', '.join(sorted(unknown))}", file=sys.stderr)
            print(f"known: {', '.join(known_modes)}", file=sys.stderr)
            return 1
        groups = {k: v for k, v in groups.items() if k[1] in args.modes}

    records: list[Record] = []

    total = sum(len(t) for t in groups.values())
    print(f"building {total} environments sequentially", flush=True)

    for (board, mode), tiers in groups.items():
        for tier in tiers:
            print(f"[{board} {mode}] tier {tier} ...", flush=True)
            rec = measure(board, mode, tier, check_flags=not args.no_flag_check)
            records.append(rec)
            if not rec.ok:
                print(f"    failed: {rec.error}", flush=True)
                # A tier-0 failure means the toolchain/board is unusable; skip the rest.
                if tier == 0:
                    print(f"    skipping remaining tiers for {board} {mode}", flush=True)
                    break

    print(render(records))

    if args.json:
        args.json.write_text(json.dumps([asdict(r) for r in records], indent=2))
        print(f"\nraw records -> {args.json}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
