#pragma once

#include "lib/imgui/imgui.h"

namespace gui {
    // DDE status window constraints
    inline constexpr float DDE_STATUS_WINDOW_WIDTH_MIN  = 220.0f;
    inline constexpr float DDE_STATUS_WINDOW_HEIGHT_MIN = 130.0f;

    // DDE status content constraints
    inline constexpr float DDE_STATUS_CONTENT_WIDTH  = 0.0f;   // Automatic width
    inline constexpr float DDE_STATUS_CONTENT_HEIGHT = 0.0f;   // Automatic height

    // DDE status colors
    inline constexpr ImVec4 DDE_STATUS_COLOR_CONNECTED     = ImVec4(0.18f, 0.64f, 0.31f, 1.0f);  // GitHub green #2da44e
    inline constexpr ImVec4 DDE_STATUS_COLOR_DISCONNECTED = ImVec4(0.81f, 0.13f, 0.18f, 1.0f);  // GitHub red #cf222e

    // DDE button colors - Connect
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_NORMAL = ImVec4(0.04f, 0.40f, 0.08f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_HOVER  = ImVec4(0.06f, 0.50f, 0.12f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_CONNECT_COLOR_ACTIVE = ImVec4(0.03f, 0.28f, 0.05f, 1.0f);

    // DDE button colors - Disconnect
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_NORMAL = ImVec4(0.55f, 0.10f, 0.08f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_HOVER  = ImVec4(0.65f, 0.14f, 0.10f, 1.0f);
    inline constexpr ImVec4 DDE_BUTTON_DISCONNECT_COLOR_ACTIVE = ImVec4(0.40f, 0.06f, 0.04f, 1.0f);

    // DDE button text
    inline constexpr ImVec4 DDE_BUTTON_TEXT_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // White

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

    // Connect DDE popup constraints
    inline constexpr float CONNECT_DDE_POPUP_WIDTH_MIN  = 260.0f;
    inline constexpr float CONNECT_DDE_POPUP_HEIGHT_MIN = 100.0f;

    // DPI scale constraints
    inline constexpr float MIN_DPI_SCALE = 1.0f;
    inline constexpr float MAX_DPI_SCALE = 5.0f;

    // Font settings
    inline constexpr float BASE_FONT_SIZE = 18.0f;

    // Update checker window constraints
    inline constexpr float UPDATE_WINDOW_WIDTH_MIN  = 320.0f;
    inline constexpr float UPDATE_WINDOW_HEIGHT_MIN = 200.0f;

    // Preferences dialog layout
    inline constexpr ImVec2 PREFERENCES_WINDOW_DEFAULT_SIZE = ImVec2(820.0f, 600.0f);
    inline constexpr float  PREFERENCES_WINDOW_MIN_WIDTH    = 640.0f;
    inline constexpr float  PREFERENCES_WINDOW_MIN_HEIGHT   = 420.0f;
    inline constexpr float  PREFERENCES_SIDEBAR_WIDTH       = 180.0f;
    inline constexpr float  PREFERENCES_FOOTER_HEIGHT      = 52.0f;
    inline constexpr float  PREFERENCES_SECTION_SPACING     = 8.0f;
}
