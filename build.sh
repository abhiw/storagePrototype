#!/bin/bash

# Script to build the StorageEngine project with various options

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/debug"

usage() {
    echo "Usage: $0 [OPTIONS] [TARGET]"
    echo ""
    echo "OPTIONS:"
    echo "  -c, --clean       Clean before building"
    echo "  -r, --release     Build in Release mode"
    echo "  -h, --help        Show this help"
    echo ""
    echo "TARGETS:"
    echo "  all              Build everything (default)"
    echo "  storage_engine   Build main executable"
    echo ""
    echo "EXAMPLES:"
    echo "  $0                 # Build debug"
    echo "  $0 --clean         # Clean and build"
    echo "  $0 --release       # Build release"
}

# Parse arguments
CLEAN=false
BUILD_TYPE="Debug"
TARGET="all"

while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            BUILD_DIR="${PROJECT_ROOT}/build/release"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            TARGET=$1
            shift
            ;;
    esac
done

# Clean if requested
if [ "$CLEAN" = true ]; then
    rm -rf "${BUILD_DIR}"
    sleep 0.5
fi

# Create build directory if it doesn't exist
mkdir -p "${BUILD_DIR}"

# Configure if needed
if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    cmake -S "${PROJECT_ROOT}" \
          -B "${BUILD_DIR}" \
          -G "Unix Makefiles" \
          -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
fi

# Build
cmake --build "${BUILD_DIR}" --target "${TARGET}"
