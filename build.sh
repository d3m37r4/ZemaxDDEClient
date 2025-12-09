#!/bin/bash

# Settings
PROJECT_NAME="ZemaxDDEClient"
BUILD_DIR="build"
SOURCE_DIR="."
OUTPUT_EXE=""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Help
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    echo "Usage: $0 [debug|release] [optimize=1|2]"
    echo "  $0               -> Release build"
    echo "  $0 debug         -> Debug build"
    echo "  $0 release       -> Release build"
    echo "  $0 optimize=2    -> Release + -O3"
    echo "  $0 debug optimize=1 -> Debug + -O2"
    exit 0
fi

# Building types
CMAKE_BUILD_TYPE="Release"
EXTRA_CMAKE_ARGS=()
EXTRA_BUILD_ARGS=()

for arg in "$@"; do
    case "$arg" in
        debug|Debug|DEBUG)
            CMAKE_BUILD_TYPE="Debug"
            ;;
        release|Release|RELEASE)
            CMAKE_BUILD_TYPE="Release"
            ;;
        optimize=1)
            EXTRA_CMAKE_ARGS+=("-DBUILD_OPTIMIZE=1")
            ;;
        optimize=2)
            EXTRA_CMAKE_ARGS+=("-DBUILD_OPTIMIZE=2")
            ;;
        --clean|-clean|clean)
            echo -e "${YELLOW}=== Cleaning build directory ===${NC}"
            rm -rf "$BUILD_DIR"
            ;;
        *)
            echo -e "${RED}Unknown argument: $arg${NC}"
            echo "Use $0 --help for usage"
            exit 1
            ;;
    esac
done

BUILD_START=$(date +%s)
echo -e "${GREEN}=== Building $PROJECT_NAME ($CMAKE_BUILD_TYPE) ===${NC}"
echo "Build started at: $(date '+%Y-%m-%d %H:%M:%S')"

# Configuration
echo -e "${YELLOW}--- Configuring with CMake ---${NC}"
cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    "${EXTRA_CMAKE_ARGS[@]}" \
    --fresh

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ CMake configuration failed${NC}"
    exit 1
fi

# Building
echo -e "${YELLOW}--- Building project ---${NC}"
cmake --build "$BUILD_DIR" "${EXTRA_BUILD_ARGS[@]}"

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Build failed${NC}"
    exit 1
fi

# Step 3: Find the newest EXE
OUTPUT_EXE=$(find "$BUILD_DIR/bin" -name "${PROJECT_NAME}_*.exe" -type f -printf '%T@ %p\n' | sort -n | tail -n1 | cut -d' ' -f2-)

if [ -z "$OUTPUT_EXE" ]; then
    echo -e "${RED}❌ Executable not found in $BUILD_DIR/bin${NC}"
    exit 1
fi

# Step 4: Output of result
BUILD_END=$(date +%s)
BUILD_TIME=$((BUILD_END - BUILD_START))

echo -e "${GREEN}=== Build successful ===${NC}"
echo "Build finished at: $(date '+%Y-%m-%d %H:%M:%S')"
echo "Elapsed time: ${BUILD_TIME}s"
echo "Output: $(pwd)/$OUTPUT_EXE"
echo "Size: $(du -h "$OUTPUT_EXE" | cut -f1)"

# Optional: run
echo -e "${YELLOW}Run with: ./$OUTPUT_EXE${NC}"
