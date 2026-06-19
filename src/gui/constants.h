#pragma once

#include "lib/imgui/imgui.h"

namespace gui {
    // DDE status window constraints
    inline constexpr ImVec2 DDE_STATUS_WINDOW_MIN_SIZE   = ImVec2(220.0f, 100.0f);
    inline constexpr ImVec2 DDE_STATUS_CONTENT_SIZE      = ImVec2(0.0f, 0.0f);   // Automatic size

    // Debug log window constraints
    inline constexpr ImVec2 DEBUG_LOG_WINDOW_MIN_SIZE    = ImVec2(236.0f, 160.0f);

    // System info window constraints
    inline constexpr ImVec2 SYSTEM_INFO_WINDOW_MIN_SIZE  = ImVec2(200.0f, 160.0f);

    // Sag analysis window constraints
    inline constexpr ImVec2 SAG_ANALYSIS_WINDOW_MIN_SIZE = ImVec2(200.0f, 160.0f);

    // Sag analysis sampling constraints
    // Maximum: Zemax tool "Surface Sag Cross Section" limit
    inline constexpr int MAX_SAMPLING = 16385;
    // Minimum: Zemax requires at least 33 for valid profile calculation
    inline constexpr int MIN_SAMPLING = 33;

    // Connect DDE popup constraints
    inline constexpr ImVec2 CONNECT_DDE_POPUP_DEFAULT_SIZE = ImVec2(550.0f, 120.0f);
    inline constexpr ImVec2 CONNECT_DDE_POPUP_MIN_SIZE     = ImVec2(550.0f, 120.0f);

    // DPI scale constraints
    inline constexpr float MIN_DPI_SCALE = 1.0f;
    inline constexpr float MAX_DPI_SCALE = 5.0f;

    // Font settings
    inline constexpr float BASE_FONT_SIZE = 18.0f;

    // Popup button dimensions (DPI-scaled at call sites)
    inline constexpr float BASE_POPUP_BUTTON_WIDTH = 120.0f;

    // Popup window names
    inline constexpr const char* ABOUT_POPUP_NAME        = "About";
    inline constexpr const char* UPDATE_POPUP_NAME       = "Check for Updates";
    inline constexpr const char* PREFERENCES_POPUP_NAME  = "Preferences";
    inline constexpr const char* CONNECT_DDE_POPUP_NAME  = "Connect to Zemax \xe2\x80\x93 select a window";

    // About popup
    inline constexpr ImVec2 ABOUT_POPUP_DEFAULT_SIZE = ImVec2(428.0f, 242.0f);
    inline constexpr ImVec2 ABOUT_POPUP_MIN_SIZE     = ImVec2(428.0f, 242.0f);

    // Check for Updates popup
    inline constexpr ImVec2 UPDATE_POPUP_DEFAULT_SIZE = ImVec2(340.0f, 120.0f);
    inline constexpr ImVec2 UPDATE_POPUP_MIN_SIZE     = ImVec2(340.0f, 120.0f);

    // Connection Lost popup
    inline constexpr ImVec2 CONNECTION_LOST_POPUP_DEFAULT_SIZE = ImVec2(380.0f, 140.0f);
    inline constexpr ImVec2 CONNECTION_LOST_POPUP_MIN_SIZE     = ImVec2(380.0f, 140.0f);

    // Preferences dialog layout
    inline constexpr ImVec2 PREFERENCES_WINDOW_DEFAULT_SIZE = ImVec2(660.0f, 280.0f);
    inline constexpr ImVec2 PREFERENCES_WINDOW_MIN_SIZE     = ImVec2(660.0f, 280.0f);
}
