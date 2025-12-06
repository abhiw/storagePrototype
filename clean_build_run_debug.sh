#!/bin/bash

# Script to clean, build in debug mode, and run the StorageEngine executable

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/debug"
EXECUTABLE="${BUILD_DIR}/storage_engine"

# Clean
if [ -d "${BUILD_DIR}" ]; then
    rm -rf "${BUILD_DIR}"
    sleep 0.5
fi

# Build
mkdir -p "${BUILD_DIR}"

cmake -S "${PROJECT_ROOT}" \
      -B "${BUILD_DIR}" \
      -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_ASAN=ON \
      -DENABLE_TESTING=OFF \
      -DENABLE_BENCHMARKS=OFF

cmake --build "${BUILD_DIR}" --target storage_engine

# Run
if [ -f "${EXECUTABLE}" ]; then
    echo ""
    "${EXECUTABLE}"
    EXIT_CODE=$?
    echo ""
    if [ $EXIT_CODE -ne 0 ]; then
        echo -e "${RED}Exit code: ${EXIT_CODE}${NC}"
        exit $EXIT_CODE
    fi
else
    echo -e "${RED}Executable not found: ${EXECUTABLE}${NC}"
    exit 1
fi
