#pragma once

namespace gui {
    // Content area constraints
    inline constexpr float CONTENT_WIDTH_MIN    = 600.0f;
    inline constexpr float CONTENT_HEIGHT_MIN   = 480.0f;

    // Debug log constraints
    inline constexpr float DEBUG_LOG_WIDTH_MIN  = 600.0f;
    inline constexpr float DEBUG_LOG_HEIGHT_MIN = 200.0f;

    // DDE status window constraints
    inline constexpr float DDE_STATUS_WINDOW_WIDTH_MIN  = 440.0f;
    inline constexpr float DDE_STATUS_WINDOW_HEIGHT_MIN = 160.0f;

    // DDE status frame constraints
    inline constexpr float DDE_STATUS_FRAME_WIDTH  = 0.0f;   // Automatic width
    inline constexpr float DDE_STATUS_FRAME_HEIGHT = 0.0f;   // Automatic height

    // Sag analysis sampling constraints
    // Maximum: Zemax tool "Surface Sag Cross Section" limit
    inline constexpr int MAX_SAMPLING = 16385;
    // Minimum: Zemax requires at least 33 for valid profile calculation
    inline constexpr int MIN_SAMPLING = 33;
}
