#!/bin/bash

cd build || exit 1

if [ "$#" -eq 0 ]; then
    ctest --output-on-failure
else
    for dir in "$@"; do
        ctest -R "${dir}" --output-on-failure
    done
fi
