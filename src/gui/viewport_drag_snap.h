#pragma once

// Snap-on-release for the floating panels: a window dragged to a monitor edge
// lands in the same half or quarter Windows itself would give it.
//
// The panel is tracked by its movement, not by the click: dragging a panel out
// of the main window creates a brand-new platform viewport in the middle of the
// drag, so a snapshot taken on mouse-down would never see that viewport.

#include <imgui.h>

#include <unordered_map>

namespace gui::placement {
    class DragSnap {
        public:
            /// Call once per frame while viewports are enabled.
            void update();

        private:
            /// Viewport position on the previous frame, to detect movement.
            std::unordered_map<ImGuiID, ImVec2> m_lastPos;
            /// Secondary viewport that moved while the left button was down.
            ImGuiID m_dragCandidate = 0;
    };
} // namespace gui::placement
