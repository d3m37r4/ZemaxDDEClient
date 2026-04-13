#pragma once

#include "lib/imgui/imgui.h"

namespace gui {
    // Content area constraints
    inline constexpr float CONTENT_WIDTH_MIN    = 600.0f;
    inline constexpr float CONTENT_HEIGHT_MIN   = 480.0f;

    // Debug log constraints
    inline constexpr float DEBUG_LOG_WIDTH_MIN  = 600.0f;
    inline constexpr float DEBUG_LOG_HEIGHT_MIN = 200.0f;

    // Sidebar constraints
    inline constexpr float SIDEBAR_WIDTH_MIN    = 248.0f;
    inline constexpr float SIDEBAR_HEIGHT_MIN   = 260.0f;

    // DDE status frame constraints
    inline constexpr float DDE_STATUS_FRAME_WIDTH  = 0.0f;   // Automatic width
    inline constexpr float DDE_STATUS_FRAME_HEIGHT = 0.0f;   // Automatic height

    // DDE status text colors
    inline constexpr ImVec4 DDE_STATUS_COLOR_CONNECTED    = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green
    inline constexpr ImVec4 DDE_STATUS_COLOR_DISCONNECTED = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red

    // DDE button colors — Connect state
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_NORMAL  = ImVec4(0.0f, 0.5f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_HOVER   = ImVec4(0.0f, 0.7f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_ACTIVE  = ImVec4(0.0f, 0.3f, 0.0f, 1.0f);

    // DDE button colors — Disconnect state
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_NORMAL = ImVec4(0.5f, 0.0f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_HOVER  = ImVec4(0.7f, 0.0f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_ACTIVE = ImVec4(0.3f, 0.0f, 0.0f, 1.0f);

    // Sag analysis sampling constraints
    // Maximum: Zemax tool "Surface Sag Cross Section" limit
    inline constexpr int MAX_SAMPLING = 16385;
    // Minimum: Zemax requires at least 33 for valid profile calculation
    inline constexpr int MIN_SAMPLING = 33;
}
