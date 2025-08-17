#!/bin/bash

# =============================================
# Configurable Variables
# =============================================
SRC_DIR="src"
IMGUI_DIR="$SRC_DIR/lib/imgui"
OUTPUT_BASE="ZemaxDDEClient"
BUILD_NUMBER=$(date +%Y%m%d%H%M%S)
OUTPUT_EXE="${OUTPUT_BASE}_${BUILD_NUMBER}.exe"

# MSYS2 paths (customize these if needed)
MSYS2_ROOT="/c/msys64"
MINGW_DIR="$MSYS2_ROOT/mingw64"
MINGW_INCLUDE="$MINGW_DIR/include"
MINGW_LIB="$MINGW_DIR/lib"

# Compiler flags
BUILD_DEBUG="${BUILD_DEBUG:-0}"             # Debug mode (1 - on, 0 - off)
BUILD_OPTIMIZE="${BUILD_OPTIMIZE:-0}"       # Optimize mode (0 - off, 1 - on with -O2, 2 - on with -O3)

# Base compiler flags by default
CXX_FLAGS="-Wall -Wextra -static-libgcc -static-libstdc++ -v -mwindows -DUNICODE -D_UNICODE"

# Adjust flags based on build modes
if [ "$BUILD_DEBUG" = "1" ]; then
    CXX_FLAGS="${CXX_FLAGS//-mwindows/}"
    CXX_FLAGS="$CXX_FLAGS -DDEBUG_LOG"
fi

# Add optimization if BUILD_OPTIMIZE is enabled
if [ "$BUILD_OPTIMIZE" = "1" ]; then
    CXX_FLAGS="$CXX_FLAGS -O2"
elif [ "$BUILD_OPTIMIZE" = "2" ]; then
    CXX_FLAGS="$CXX_FLAGS -O3"
fi

LINK_FLAGS="-lglfw3 -lopengl32 -lgdi32 -luser32 -limm32"

# =============================================
# Source Files Configuration
# =============================================
SOURCE_FILES=(
    "$SRC_DIR/main.cpp"
    "$SRC_DIR/dde/dde_zemax_client_core.cpp"
    "$SRC_DIR/dde/dde_zemax_client_data.cpp"
    "$SRC_DIR/dde/dde_zemax_utils.cpp"
    "$SRC_DIR/gui/gui_utils.cpp"
    "$SRC_DIR/gui/components/gui_debug_log.cpp"
    "$SRC_DIR/gui/components/gui_popups.cpp"
    "$SRC_DIR/gui/components/gui_dde_status.cpp"
    "$SRC_DIR/gui/components/gui_sidebar.cpp"
    "$SRC_DIR/gui/components/gui_menu_bar.cpp"
    "$SRC_DIR/gui/components/gui_content.cpp"
    "$SRC_DIR/gui/content_pages/gui_page_optical_system_info.cpp"
    "$SRC_DIR/gui/content_pages/gui_page_local_surface_errors.cpp"
    "$SRC_DIR/gui/gui_init.cpp"
    "$SRC_DIR/gui/gui_container.cpp"
    "$SRC_DIR/logger/logger.cpp"
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
        -I"$SRC_DIR/gui" \
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
