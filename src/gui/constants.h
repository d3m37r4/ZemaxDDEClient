#pragma once

#include "lib/imgui/imgui.h"

namespace gui {
    // DDE status window constraints
    inline constexpr float DDE_STATUS_WINDOW_WIDTH_MIN  = 220.0f;
    inline constexpr float DDE_STATUS_WINDOW_HEIGHT_MIN = 100.0f;

    // DDE status content constraints
    inline constexpr float DDE_STATUS_CONTENT_WIDTH  = 0.0f;   // Automatic width
    inline constexpr float DDE_STATUS_CONTENT_HEIGHT = 0.0f;   // Automatic height

    // DDE status colors
    inline constexpr ImVec4 DDE_STATUS_COLOR_CONNECTED     = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green
    inline constexpr ImVec4 DDE_STATUS_COLOR_DISCONNECTED = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red

    // DDE button colors - Connect
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_NORMAL = ImVec4(0.0f, 0.5f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_HOVER   = ImVec4(0.0f, 0.7f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_ACTIVE  = ImVec4(0.0f, 0.3f, 0.0f, 1.0f);

    // DDE button colors - Disconnect
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_NORMAL = ImVec4(0.5f, 0.0f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_HOVER  = ImVec4(0.7f, 0.0f, 0.0f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_ACTIVE = ImVec4(0.3f, 0.0f, 0.0f, 1.0f);

    // Debug log window constraints
    inline constexpr float DEBUG_LOG_WINDOW_WIDTH_MIN  = 220.0f;
    inline constexpr float DEBUG_LOG_WINDOW_HEIGHT_MIN = 160.0f;

    // System info window constraints
    inline constexpr float SYSTEM_INFO_WINDOW_WIDTH_MIN  = 200.0f;
    inline constexpr float SYSTEM_INFO_WINDOW_HEIGHT_MIN = 160.0f;

    // Sag analysis window constraints
    inline constexpr float SAG_ANALYSIS_WINDOW_WIDTH_MIN  = 200.0f;
    inline constexpr float SAG_ANALYSIS_WINDOW_HEIGHT_MIN = 160.0f;

    // Sag analysis sampling constraints
    // Maximum: Zemax tool "Surface Sag Cross Section" limit
    inline constexpr int MAX_SAMPLING = 16385;
    // Minimum: Zemax requires at least 33 for valid profile calculation
    inline constexpr int MIN_SAMPLING = 33;

    // DPI scale constraints
    inline constexpr float MIN_DPI_SCALE = 1.0f;
    inline constexpr float MAX_DPI_SCALE = 5.0f;

    // Font settings
    inline constexpr float BASE_FONT_SIZE = 18.0f;

    // Update checker window constraints
    inline constexpr float UPDATE_WINDOW_WIDTH_MIN  = 320.0f;
    inline constexpr float UPDATE_WINDOW_HEIGHT_MIN = 200.0f;
}
