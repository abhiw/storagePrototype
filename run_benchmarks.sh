#!/bin/bash

# Script to build and run benchmarks for StorageEngine project

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/debug"

# Configure if needed
if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    cmake -S "${PROJECT_ROOT}" \
          -B "${BUILD_DIR}" \
          -G "Unix Makefiles" \
          -DCMAKE_BUILD_TYPE=Debug \
          -DENABLE_BENCHMARKS=ON
fi

# Build the project
cmake --build "${BUILD_DIR}" --target all

# Check if benchmark executables exist
if [ -d "${BUILD_DIR}/benchmarks" ]; then
    BENCHMARK_FOUND=false
    for bench in "${BUILD_DIR}"/benchmarks/*_benchmark; do
        if [ -f "$bench" ] && [ -x "$bench" ]; then
            BENCHMARK_FOUND=true
            "$bench"
        fi
    done

    if [ "$BENCHMARK_FOUND" = false ]; then
        echo -e "${YELLOW}No benchmark executables found${NC}"
    fi
else
    echo -e "${YELLOW}No benchmarks configured${NC}"
    echo "To add benchmarks, uncomment 'add_subdirectory(benchmarks)' in CMakeLists.txt"
fi
