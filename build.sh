#!/bin/bash

# =============================================
# Configurable Variables
# =============================================
SRC_DIR="src"
IMGUI_DIR="$SRC_DIR/imgui"
OUTPUT_BASE="ZemaxDDEClient"
BUILD_NUMBER=$(date +%Y%m%d%H%M%S)
OUTPUT_EXE="${OUTPUT_BASE}_${BUILD_NUMBER}.exe"

# MSYS2 paths (customize these if needed)
MSYS2_ROOT="/c/msys64"
MINGW_DIR="$MSYS2_ROOT/mingw64"
MINGW_INCLUDE="$MINGW_DIR/include"
MINGW_LIB="$MINGW_DIR/lib"

# Compiler flags
CXX_FLAGS="-Wall -Wextra -static-libgcc -static-libstdc++ -v -DUNICODE -D_UNICODE"
LINK_FLAGS="-lglfw3 -lopengl32 -lgdi32 -luser32"

# =============================================
# Source Files Configuration
# =============================================
SOURCE_FILES=(
    "$SRC_DIR/main.cpp"
    "$SRC_DIR/dde_client.cpp"
    # "$SRC_DIR/zemax_dde.h"
    # "$SRC_DIR/zemax_dde.c"
    "$IMGUI_DIR/imgui.cpp"
    "$IMGUI_DIR/imgui_draw.cpp"
    "$IMGUI_DIR/imgui_tables.cpp"
    "$IMGUI_DIR/imgui_widgets.cpp"
    "$IMGUI_DIR/backends/imgui_impl_glfw.cpp"
    "$IMGUI_DIR/backends/imgui_impl_opengl3.cpp"
)

# =============================================
# Build Process
# =============================================

# Check environment
check_environment() {
    # Verify MSYS2 paths
    if [ ! -d "$MINGW_INCLUDE" ]; then
        echo "ERROR: MinGW include directory not found at $MINGW_INCLUDE"
        echo "Please set correct MSYS2_ROOT in build.sh or install MSYS2"
        exit 1
    fi

    # Verify ImGui
    if [ ! -d "$IMGUI_DIR" ]; then
        echo "ERROR: ImGui directory not found at $IMGUI_DIR"
        echo "Did you forget to init submodules? Run:"
        echo "  git submodule update --init --recursive"
        exit 1
    fi
}

# Verify source files
check_source_files() {
    for file in "${SOURCE_FILES[@]}"; do
        if [ ! -f "$file" ]; then
            echo "ERROR: Source file not found: $file"
            exit 1
        fi
    done
}

# Main build function
build_project() {
    echo "=== Building $OUTPUT_EXE ==="
    echo "Compiler: $(g++ --version | head -n1)"
    echo "Build timestamp: $(date)"
    
    g++ -o "$OUTPUT_EXE" \
        "${SOURCE_FILES[@]}" \
        -I"$SRC_DIR" \
        -I"$IMGUI_DIR" \
        -I"$IMGUI_DIR/backends" \
        -I"$MINGW_INCLUDE" \
        -L"$MINGW_LIB" \
        $CXX_FLAGS \
        $LINK_FLAGS
    
    if [ $? -eq 0 ]; then
        echo "=== Build successful ==="
        echo "Output: $(pwd)/$OUTPUT_EXE"
        echo "Size: $(du -h "$OUTPUT_EXE" | cut -f1)"
    else
        echo "=== Build failed ==="
        exit 1
    fi
}

# =============================================
# Main Execution
# =============================================
check_environment
check_source_files
build_project
