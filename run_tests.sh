#!/bin/bash

# Script to build and run tests for StorageEngine project

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build_test"

# Configure if needed
if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    cmake -S "${PROJECT_ROOT}" \
          -B "${BUILD_DIR}" \
          -G "Unix Makefiles" \
          -DCMAKE_BUILD_TYPE=Debug \
          -DENABLE_TESTING=ON
fi

# Build the project
cmake --build "${BUILD_DIR}" --target all

# Check if test executables exist
if [ -d "${BUILD_DIR}/test" ]; then
    cd "${BUILD_DIR}"
    ctest --output-on-failure
else
    echo -e "${YELLOW}No tests configured${NC}"
    echo "To add tests, uncomment 'add_subdirectory(test)' in CMakeLists.txt"
fi
