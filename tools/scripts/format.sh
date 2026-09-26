#!/usr/bin/env bash

set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

echo "Formatting C++..."

find "$ROOT_DIR" \
    \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
    -not -path "$ROOT_DIR/build/*" \
    -not -path "$ROOT_DIR/external/*" \
    -exec clang-format -i {} +

echo "Formatting CMake..."

find "$ROOT_DIR" \
    \( -name "CMakeLists.txt" -o -name "*.cmake" \) \
    -not -path "$ROOT_DIR/build/*" \
    -not -path "$ROOT_DIR/external/*" \
    -type f \
    -exec cmake-format -i {} +

echo "Formatting complete."
