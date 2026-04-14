#pragma once

// gui_impl.h — internal implementation header.
// Includes all heavy dependencies required for GuiManager implementation.
// NOT for use in component files; use gui/gui.h instead.

#include <filesystem>
#include <vector>

#include <windows.h>
#include <GLFW/glfw3.h>

#include <nfd.h>
#include "lib/imgui/imgui.h"
#include "lib/imgui/backends/imgui_impl_glfw.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"
#include "lib/implot/implot.h"

#include "gui/sag_analysis_service.h"
#include "gui/gui.h"
