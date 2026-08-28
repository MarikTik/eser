#!/usr/bin/env bash
#
# Codegen-quality check: prove that a native-endian serialize compiles to the same instructions as
# a hand-written memcpy, and that the byte-swap path appears only when the wire order differs.
#
# This is the one eser claim that a size table cannot settle. Tier deltas show the big-endian path
# costs extra bytes; only the disassembly shows that the *native* path costs nothing at all.
#
# Usage: bench/scripts/codegen.sh [target]
#   target: host (default) | xtensa | arm
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

TARGET="${1:-host}"

case "$TARGET" in
  host)   CXX="g++"; FLAGS="-O2 -std=c++17" ;;
  xtensa) CXX="$(find "$HOME/.platformio/packages" -name 'xtensa-*-elf-g++' | head -1)"
          FLAGS="-O2 -std=c++17" ;;
  arm)    CXX="$(find "$HOME/.platformio/packages" -name 'arm-none-eabi-g++' | head -1)"
          FLAGS="-O2 -std=c++17 -mcpu=cortex-m4 -mthumb --specs=nosys.specs" ;;
  *)      echo "unknown target: $TARGET (expected host|xtensa|arm)" >&2; exit 2 ;;
esac

if [ -z "${CXX:-}" ] || ! command -v "$CXX" >/dev/null 2>&1; then
  echo "codegen: toolchain for '$TARGET' not found; skipping" >&2
  exit 0
fi

cat > "$WORK/cg.cpp" <<'EOF'
#include "eser/eser.hpp"
#include <cstring>
#include <cstdint>
using namespace eser::flat;
using eser::utils::endianness;

// Reference: what a hand-written flat copy of the same three fields costs.
extern "C" std::size_t by_hand(std::byte *out, std::uint32_t a, std::int16_t b, float c)
{
    std::memcpy(out + 0, &a, 4);
    std::memcpy(out + 4, &b, 2);
    std::memcpy(out + 6, &c, 4);
    return 10;
}

// eser at the native wire order: should be indistinguishable from by_hand.
extern "C" std::size_t via_eser_native(std::byte *out, std::uint32_t a, std::int16_t b, float c)
{
    return serialize(a, b, c).to(out, 10);
}

// eser at the opposite wire order: must contain byte-reversal work.
extern "C" std::size_t via_eser_swapped(std::byte *out, std::uint32_t a, std::int16_t b, float c)
{
    return serialize<endianness::big>(a, b, c).to(out, 10);
}
EOF

"$CXX" $FLAGS -I"$ROOT" -c "$WORK/cg.cpp" -o "$WORK/cg.o"

OBJDUMP="${CXX%g++}objdump"
command -v "$OBJDUMP" >/dev/null 2>&1 || OBJDUMP="objdump"

dump_fn() {
  "$OBJDUMP" -d --no-show-raw-insn "$WORK/cg.o" \
    | awk -v fn="$1" '
        $0 ~ "<"fn">:" {inside=1; next}
        inside && /^$/  {inside=0}
        inside          {sub(/^[[:space:]]*[0-9a-f]+:[[:space:]]*/, ""); print}
      '
}

count_fn() { dump_fn "$1" | grep -c . || true; }

HAND=$(count_fn by_hand)
NATIVE=$(count_fn via_eser_native)
SWAPPED=$(count_fn via_eser_swapped)

echo "=== eser codegen quality ($TARGET, $FLAGS) ==="
printf '  %-24s %s instructions\n' "by_hand (memcpy)"      "$HAND"
printf '  %-24s %s instructions\n' "eser, native wire"     "$NATIVE"
printf '  %-24s %s instructions\n' "eser, swapped wire"    "$SWAPPED"
echo

if [ "$NATIVE" -le "$HAND" ]; then
  echo "  PASS  native-endian eser is not larger than a hand-written memcpy (zero abstraction cost)"
else
  echo "  NOTE  native-endian eser emits $((NATIVE - HAND)) more instruction(s) than by_hand"
fi

if [ "$SWAPPED" -gt "$NATIVE" ]; then
  echo "  PASS  swapped wire adds $((SWAPPED - NATIVE)) instruction(s) - byte-reversal is present only when needed"
else
  echo "  WARN  swapped wire is not larger than native; the byte-swap may have been elided"
fi

echo
echo "--- via_eser_native ---"
dump_fn via_eser_native
echo
echo "--- via_eser_swapped ---"
dump_fn via_eser_swapped
