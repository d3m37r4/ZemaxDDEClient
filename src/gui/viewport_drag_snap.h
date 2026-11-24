#pragma once

// Snap-on-release for the floating OS windows: a secondary window dragged to a
// monitor edge lands in the same half or quarter Windows itself would give it.
// The window is moved by ImGui while it is being dragged, so the zone is only
// applied once, on release, instead of fighting that drag every frame.

#include <imgui.h>

#include <unordered_map>

namespace gui::placement {
    class DragSnap {
        public:
            /// Call once per frame while viewports are enabled, before
            /// ImGui::UpdatePlatformWindows().
            void update();

        private:
            /// Window position at the moment the left button went down.
            std::unordered_map<ImGuiID, ImVec2> m_pressPos;
    };
} // namespace gui::placement
