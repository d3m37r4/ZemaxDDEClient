#pragma once

// Floating-window chrome: title-bar minimize/maximize buttons plus the
// window shelf ("adaptive taskbar") that collects minimized windows at the
// bottom-left of the dock area.
//
// One ImGui window renders either merged into the main viewport or as its
// own platform window (multi-viewport): the chrome lives with the window
// and behaves identically in both modes.
//
// All state is keyed by a caller-stable token pointer, NOT by ImGui window
// IDs: titles embed dynamic parts ('[Slot N]', angles), so a window ID
// changes on recalculations and cannot carry state. Detached plots pass
// their &open-bool (stable member addresses, see renderToolbarWindow);
// dockable panels pass the WindowID value cast through tokenFor().

#include "imgui.h"

#include <string>
#include <string_view>

namespace gui::chrome {

/// Per-logical-window state: the floating rect to come back to plus flags.
struct State {
    bool minimized = false;
    bool maximized = false;
    bool wantRestoreRect = false;
    ImVec2 restorePos{0.0f, 0.0f};
    ImVec2 restoreSize{0.0f, 0.0f};
};

/// Registers the shelf tile when the window is minimized. Returns true when
/// the caller must skip the whole rendering (Begin/End) for this frame.
/// Call first, before any SetNextWindow* of this window, so pending data
/// cannot leak into the next one; prunes stale tiles automatically.
bool minimizedAndShelved(const void* token, std::string_view title);

/// Forces the window rect while maximized (dock area, below the menu bar,
/// above the status bar + shelf) and once after un-minimize/un-maximize
/// restores the saved floating rect. Must be the LAST SetNextWindow* call
/// before Begin() so its ImGuiCond_Always wins over queued Once conditions.
void applyFrameRect(const void* token);

/// Draws the - and full-screen (maximize/restore) buttons in the current
/// window's title bar, left of the existing x close button. Skips docked
/// windows: their geometry belongs to the dock node, and a stale floating
/// maximized state from a floating period is cleared there.
/// Call right after Begin() returned true.
void drawTitleBarButtons(const void* token, bool* open);

/// Maximization query for ClampWindowToViewport skips: a maximized window
/// already spans the dock area exactly and must not be clamped.
bool isMaximized(const void* token);

/// Dock-bottom block height (status bar + shelf) used by the maximized
/// rect; the status bar reports it every frame via setBottomBlockHeight().
void setBottomBlockHeight(float height);

/// Height of the shelf strip (0 when no window is minimized). Prunes shelf
/// entries for windows that stopped minimizing over small frame gaps, so
/// paused/hidden windows disappear from the shelf without explicit hooks.
float currentShelfHeight();

/// Renders the shelf itself: an invisible NoBackground window right above
/// the status bar holding one tile per minimized window. Tiles share the
/// available width (Windows taskbar behavior: every added tile narrows all
/// the others), overflow wraps to extra lines that grow the dock-bottom
/// block. A tile click restores its window at the saved floating rect.
/// Call once per frame, after the regular windows have been rendered.
void renderShelf();

} // namespace gui::chrome
