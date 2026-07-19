#!/bin/bash
# Build and run the offline correction measurement harness (no JUCE link required).
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
clang++ -std=c++17 -O2 -I "$DIR" "$DIR/measure.cpp" -o "$DIR/measure"
"$DIR/measure"
