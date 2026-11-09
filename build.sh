#!/bin/bash
set -e

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

if [ "$#" -gt 0 ]; then
    ./build/output "$@"
fi