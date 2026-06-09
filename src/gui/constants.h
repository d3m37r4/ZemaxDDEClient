#pragma once

#include "lib/imgui/imgui.h"

namespace gui {
    // DDE status window constraints
    inline constexpr float DDE_STATUS_WINDOW_WIDTH_MIN  = 220.0f;
    inline constexpr float DDE_STATUS_WINDOW_HEIGHT_MIN = 130.0f;

    // DDE status content constraints
    inline constexpr float DDE_STATUS_CONTENT_WIDTH  = 0.0f;   // Automatic width
    inline constexpr float DDE_STATUS_CONTENT_HEIGHT = 0.0f;   // Automatic height

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
    inline constexpr float  PREFERENCES_FOOTER_HEIGHT      = 88.0f;
    inline constexpr float  PREFERENCES_SECTION_SPACING     = 8.0f;
}
