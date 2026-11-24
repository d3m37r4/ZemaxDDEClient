#pragma once

// Screen geometry for placing the floating OS windows the way Windows does it:
// halves and quarters of a monitor's work area, and the edge zones that start a
// snap when the mouse is released there. Pure math, no ImGui and no Win32, so
// the rules can be unit-tested without a desktop.

#include <optional>

namespace gui::placement {
    /// Rectangle in physical screen pixels, the space GLFW and the work area
    /// of a monitor use.
    struct Rect {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        bool operator==(const Rect&) const = default;
    };

    enum class Target {
        Maximize,
        LeftHalf,
        RightHalf,
        TopLeftQuarter,
        TopRightQuarter,
        BottomLeftQuarter,
        BottomRightQuarter,
    };

    /// Rectangle a window takes for @p target inside @p work. Halves and
    /// quarters cover the work area exactly, without gaps on odd sizes.
    Rect rectFor(const Rect& work, Target target);

    /// Zone under the screen point (@p px, @p py), or nothing when the point is
    /// away from the work area edges or outside it. @p margin is the edge
    /// thickness in pixels, so a corner and an edge cannot overlap on tiny
    /// monitors.
    std::optional<Target> edgeZoneAt(const Rect& work, int px, int py, int margin);
} // namespace gui::placement
