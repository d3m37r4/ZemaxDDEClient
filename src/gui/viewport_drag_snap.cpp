#include "gui/viewport_drag_snap.h"

#include "gui/placement_geometry.h"
#include "gui/viewport_placement.h"

namespace gui::placement {
    namespace {
        /// Edge thickness that starts a snap, in DIP (96 DPI pixels).
        constexpr float kSnapMarginDip = 8.0f;
    } // namespace

    void DragSnap::update() {
        pruneSavedRects();

        ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        ImGuiPlatformIO& platform = ImGui::GetPlatformIO();

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_pressPos.clear();
            for (ImGuiViewport* viewport : platform.Viewports) {
                if (viewport->ID != mainViewport->ID) {
                    m_pressPos.emplace(viewport->ID, viewport->Pos);
                }
            }
        }
        if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            return;
        }

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const int px = static_cast<int>(mouse.x);
        const int py = static_cast<int>(mouse.y);
        const Rect work = workAreaAt(px, py);
        if (work.w <= 0 || work.h <= 0) {
            m_pressPos.clear();
            return;
        }
        // Same DPI factor the GUI uses for every size: style.FontScaleDpi is
        // what updateDpiStyle() writes (the app scales through it, not through
        // viewport DPI).
        const float dpiScale = ImGui::GetStyle().FontScaleDpi > 0.0f
            ? ImGui::GetStyle().FontScaleDpi : 1.0f;
        const int margin = static_cast<int>(kSnapMarginDip * dpiScale);
        const std::optional<Target> zone = edgeZoneAt(work, px, py, margin);

        for (ImGuiViewport* viewport : platform.Viewports) {
            const auto pressed = m_pressPos.find(viewport->ID);
            if (pressed == m_pressPos.end()) {
                continue;
            }
            // A move started elsewhere (layout restore, our own apply()) is not
            // a drag: only a position that changed while the button was down
            // may snap.
            const bool dragged = pressed->second.x != viewport->Pos.x ||
                                 pressed->second.y != viewport->Pos.y;
            if (!dragged || viewport->PlatformHandle == nullptr) {
                continue;
            }
            if (zone.has_value()) {
                requestPlacement(viewport, rectFor(work, *zone));
            }
        }
        m_pressPos.clear();
    }
} // namespace gui::placement
