#!/bin/bash

# Variables
SOURCE_FILES="main.cpp zemax_dde.c imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp"
OUTPUT_BASE="ZemaxDDEClient"
BUILD_NUMBER=$(date +%Y%m%d%H%M%S)
OUTPUT_EXE="${OUTPUT_BASE}_${BUILD_NUMBER}.exe"
IMGUI_DIR="imgui"

# Check ImGui
if [ ! -d "$IMGUI_DIR" ]; then
    echo "Error: ImGui directory not found at $IMGUI_DIR."
    exit 1
fi

# Check source files
for file in $SOURCE_FILES; do
    if [ ! -f "$file" ]; then
        echo "Error: $file not found."
        exit 1
    fi
done

# Compile
echo "Compiling $OUTPUT_EXE..."
g++ -o "$OUTPUT_EXE" $SOURCE_FILES -I"$IMGUI_DIR" -I"$IMGUI_DIR/backends" -I/c/msys64/mingw64/include -L/c/msys64/mingw64/lib -lglfw3 -lopengl32 -lgdi32 -luser32 -static-libgcc -static-libstdc++ -Wall -v
if [ $? -ne 0 ]; then
    echo "Error: Compilation failed."
    exit 1
fi

echo "Compilation successful! $OUTPUT_EXE created."
