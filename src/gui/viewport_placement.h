#pragma once

// Placement of the floating panels that live in their own OS window. The ImGui
// window is the source of truth: the helpers compute a rectangle in screen
// pixels and the chrome applies it to the ImGui window, after which ImGui moves
// and resizes the platform window itself. Driving the OS window directly left
// viewport->Pos stale, and io.MousePos is derived from it, so the panel hit
// areas drifted away from the cursor.

#include <imgui.h>

#include "gui/placement_geometry.h"

namespace gui::placement {
    /// Work area (the taskbar and other appbars excluded) of the monitor under
    /// the screen point (@p x, @p y).
    Rect workAreaAt(int x, int y);

    /// Work area of the monitor the viewport's own OS window is on.
    Rect workAreaOf(const ImGuiViewport* viewport);

    /// Queues a rectangle for the panel of @p viewport. The chrome of that panel
    /// picks it up on the next frame and applies it to the ImGui window.
    void requestPlacement(ImGuiViewport* viewport, const Rect& rect);

    /// Consumes the queued rectangle, if any.
    bool takePlacement(ImGuiViewport* viewport, Rect& out);

    /// Saves @p current and queues the work area, or queues the saved rect back
    /// when the panel is already maximized. @p current is the panel's screen
    /// rect (position and full size).
    void toggleMaximize(ImGuiViewport* viewport, const Rect& current);

    /// True while a maximized rect is pending or applied.
    bool isMaximized(const ImGuiViewport* viewport);

    /// Drops the saved rects of windows that no longer exist. Platform viewport
    /// IDs are reused, and a stale entry would make the first maximize click of
    /// a new window restore instead.
    void pruneSavedRects();
} // namespace gui::placement
