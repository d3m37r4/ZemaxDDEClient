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
        const bool buttonDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

        // Which secondary window the user is actually moving. Following the
        // movement instead of the click covers the panel that is dragged out of
        // the main window: ImGui hands it a new platform viewport mid-drag, and
        // that viewport did not exist when the button went down.
        for (ImGuiViewport* viewport : platform.Viewports) {
            if (viewport->ID == mainViewport->ID) {
                continue;
            }
            const auto previous = m_lastPos.find(viewport->ID);
            if (previous == m_lastPos.end()) {
                m_lastPos.emplace(viewport->ID, viewport->Pos);
                continue;
            }
            if (buttonDown && (previous->second.x != viewport->Pos.x ||
                               previous->second.y != viewport->Pos.y)) {
                m_dragCandidate = viewport->ID;
            }
            previous->second = viewport->Pos;
        }

        if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            return;
        }
        const ImGuiID candidate = m_dragCandidate;
        m_dragCandidate = 0;
        m_lastPos.clear();

        if (candidate == 0) {
            return;
        }
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const int px = static_cast<int>(mouse.x);
        const int py = static_cast<int>(mouse.y);
        const Rect work = workAreaAt(px, py);
        if (work.w <= 0 || work.h <= 0) {
            return;
        }
        // Same DPI factor the GUI uses for every size: style.FontScaleDpi is
        // what updateDpiStyle() writes (the app scales through it, not through
        // viewport DPI).
        const float dpiScale = ImGui::GetStyle().FontScaleDpi > 0.0f
            ? ImGui::GetStyle().FontScaleDpi : 1.0f;
        const int margin = static_cast<int>(kSnapMarginDip * dpiScale);
        const std::optional<Target> zone = edgeZoneAt(work, px, py, margin);
        if (!zone.has_value()) {
            return;
        }
        if (ImGuiViewport* viewport = ImGui::FindViewportByID(candidate)) {
            requestPlacement(viewport, rectFor(work, *zone));
        }
    }
} // namespace gui::placement
