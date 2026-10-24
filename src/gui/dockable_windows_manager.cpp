#include "dockable_windows_manager.h"
#include "gui.h"
#include "gui/utils.h"
#include "gui/imgui_utils.h"
#include "app/config_path.h"
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

DockableWindowsManager::DockableWindowsManager() = default;
DockableWindowsManager::~DockableWindowsManager() = default;

void DockableWindowsManager::RegisterWindow(WindowID id, const char* name, WindowCategory category, std::function<void()> renderFn, int order, bool isVisible) {
    WindowEntry entry;
    entry.id = id;
    entry.category = category;
    entry.name = name;
    entry.render = std::move(renderFn);
    entry.isVisible = isVisible;
    entry.order = order;

    windows_.push_back(entry);
}

bool DockableWindowsManager::IsVisible(WindowID id) const {
    for (const auto& w : windows_) {
        if (w.id == id) return w.isVisible;
    }
    return false;
}

void DockableWindowsManager::SetVisible(WindowID id, bool visible) {
    for (auto& w : windows_) {
        if (w.id == id) {
            w.isVisible = visible;
            break;
        }
    }
}

void DockableWindowsManager::RenderAll() {
    for (auto& w : windows_) {
        if (w.isVisible && w.render) {
            w.render();
        }
    }
}

void DockableWindowsManager::LoadState() {
    std::ifstream in(app::getWindowStatePath());
    if (!in) return;
    try {
        nlohmann::json j; in >> j;
        for (auto& el : j.items()) {
            const std::string& name = el.key();
            bool visible = el.value().get<bool>();
            for (auto& w : windows_) {
                if (w.name == name) {
                    w.isVisible = visible;
                    break;
                }
            }
        }
    } catch (const std::exception&) {
    }
}

void DockableWindowsManager::SaveState() {
    nlohmann::json j;
    for (const auto& w : windows_) {
        j[w.name] = w.isVisible;
    }
    std::ofstream out(app::getWindowStatePath());
    out << j.dump(4);
}

const char* DockableWindowsManager::GetName(WindowID id) const {
    for (const auto& w : windows_) {
        if (w.id == id) return w.name.c_str();
    }
    return nullptr;
}

namespace {
    void RenderDDEStatusWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        auto* mgr = guiMgr->getWindowManager();
        bool isVisible = mgr->IsVisible(WindowID::DDEStatus);
        const char* title = mgr->GetName(WindowID::DDEStatus);
        ImGuiUtils::SetDpiScaledWindowConstraints(gui::DDE_STATUS_WINDOW_MIN_SIZE.x, gui::DDE_STATUS_WINDOW_MIN_SIZE.y);
        if (ImGui::Begin(title, &isVisible,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            if (guiMgr->getDDEStatusRenderer()) {
                guiMgr->getDDEStatusRenderer()->render(guiMgr->getLogger());
            }
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::DDEStatus, false);
        }
    }

    void RenderSystemInfoWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        auto* mgr = guiMgr->getWindowManager();
        bool isVisible = mgr->IsVisible(WindowID::SystemInfo);
        const char* title = mgr->GetName(WindowID::SystemInfo);
        ImGuiUtils::SetDpiScaledWindowConstraints(gui::SYSTEM_INFO_WINDOW_MIN_SIZE.x, gui::SYSTEM_INFO_WINDOW_MIN_SIZE.y);
        if (ImGui::Begin(title, &isVisible)) {
            guiMgr->renderOpticalSystemInfo();
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::SystemInfo, false);
        }
    }

    void RenderSurfaceProfileInspectorWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        auto* mgr = guiMgr->getWindowManager();
        bool isVisible = mgr->IsVisible(WindowID::SurfaceProfileInspector);
        const char* title = mgr->GetName(WindowID::SurfaceProfileInspector);
        ImGuiUtils::SetDpiScaledWindowConstraints(gui::SAG_ANALYSIS_WINDOW_MIN_SIZE.x, gui::SAG_ANALYSIS_WINDOW_MIN_SIZE.y);
        if (ImGui::Begin(title, &isVisible)) {
            guiMgr->renderSurfaceProfileInspector();
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::SurfaceProfileInspector, false);
        }
    }

    void RenderSurfaceIrregularityMapWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        auto* mgr = guiMgr->getWindowManager();
        bool isVisible = mgr->IsVisible(WindowID::SurfaceIrregularityMap);
        const char* title = mgr->GetName(WindowID::SurfaceIrregularityMap);
        ImGuiUtils::SetDpiScaledWindowConstraints(gui::SAG_ANALYSIS_WINDOW_MIN_SIZE.x, gui::SAG_ANALYSIS_WINDOW_MIN_SIZE.y);
        if (ImGui::Begin(title, &isVisible)) {
            guiMgr->renderSurfaceIrregularityMap();
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::SurfaceIrregularityMap, false);
        }
    }

    void RenderLogsWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        auto* mgr = guiMgr->getWindowManager();
        bool isVisible = mgr->IsVisible(WindowID::Logs);
        const char* title = mgr->GetName(WindowID::Logs);
        ImGuiUtils::SetDpiScaledWindowConstraints(gui::LOGS_WINDOW_MIN_SIZE.x, gui::LOGS_WINDOW_MIN_SIZE.y);
        if (ImGui::Begin(title, &isVisible)) {
            guiMgr->renderLogs();
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::Logs, false);
        }
    }

    void RenderWelcomeWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        auto* mgr = guiMgr->getWindowManager();
        // The card is only meaningful while the DDE link is down. The
        // visibility flag IS the View menu checkmark: hide the card on connect
        // (unchecks the menu), restore it when the link goes down again so the
        // hint re-appears centered (Appearing cond handles the placement).
        static bool wasConnected = false;
        if (guiMgr->isDDEInitialized()) {
            if (mgr->IsVisible(WindowID::Welcome)) {
                mgr->SetVisible(WindowID::Welcome, false);
            }
            wasConnected = true;
            return;
        }
        if (wasConnected) {
            wasConnected = false;
            mgr->SetVisible(WindowID::Welcome, true);
        }
        // Dock area rect: the centering target and the clamp reference.
        const float dockTop = ImGui::GetFrameHeight();
        const float dockHeight = ImGui::GetIO().DisplaySize.y - dockTop - guiMgr->computeStatusBarHeight();
        const ImVec2 dockAreaMin(0.0f, dockTop);
        const ImVec2 dockAreaMax(ImGui::GetIO().DisplaySize.x, dockTop + dockHeight);
        const ImVec2 dockAreaCenter((dockAreaMin.x + dockAreaMax.x) * 0.5f,
                                    (dockAreaMin.y + dockAreaMax.y) * 0.5f);

        // Visibility may persist across a viewport resize (maximize/restore):
        // the ini-stored or dragged position then points to a stale rect.
        // Cached extents from the previous frame reveal that drift; a pixel
        // slack keeps manual moves fully inside the dock area from flipping.
        static ImVec2 lastPos(FLT_MAX, FLT_MAX);
        static ImVec2 lastSize(-1.0f, -1.0f);
        static bool renderedLastFrame = false;
        // Stale-but-inside is not caught by the bounds check above: a
        // maximize/restore monitor swap leaves a manually placed card fully
        // visible yet off center. So a viewport size change forces a recenter
        // for that frame (same technique as the popups in CenterNextWindow).
        static ImVec2 lastViewportSize(-1.0f, -1.0f);
        const ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
        bool viewportResized = renderedLastFrame &&
            (viewportSize.x != lastViewportSize.x || viewportSize.y != lastViewportSize.y);
        constexpr float slack = 1.0f;
        bool outOfDock = lastSize.x >= 0.0f &&
            (lastPos.x - slack < dockAreaMin.x || lastPos.y - slack < dockAreaMin.y ||
             lastPos.x + lastSize.x + slack > dockAreaMax.x ||
             lastPos.y + lastSize.y + slack > dockAreaMax.y);

        // Recenter at every appearance (app start, View reopen, return after
        // disconnect) and unconditionally on detected drift or viewport change.
        ImGui::SetNextWindowPos(dockAreaCenter,
            (outOfDock || viewportResized) ? ImGuiCond_Always : ImGuiCond_Appearing,
            ImVec2(0.5f, 0.5f));

        bool isVisible = mgr->IsVisible(WindowID::Welcome);
        const char* title = mgr->GetName(WindowID::Welcome);
        // Content-fitted card: no resize, no scrolling. Extra window padding
        // keeps the text off the borders on both sides.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImGuiUtils::DpiScaleVec2(gui::WELCOME_CARD_PADDING));
        // Deterministic size: pivot centering is computed from the size given
        // here - AlwaysAutoResize would settle one frame later and drag the
        // appearing position off the center (seen as a half-size shift after
        // maximize/restore or on a move between DPI zones).
        ImGui::SetNextWindowSize(guiMgr->welcomeWindowSize());
        // NoDocking: floating helper, not a data panel. NoCollapse: the card
        // never shrinks to a bare title bar, keeping the geometry math intact.
        if (ImGui::Begin(title, &isVisible,
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse)) {
            lastPos = ImGui::GetWindowPos();
            lastSize = ImGui::GetWindowSize();
            lastViewportSize = viewportSize;
            renderedLastFrame = true;
            guiMgr->renderWelcomeContent();
        } else {
            renderedLastFrame = false;
        }
        ImGui::End();
        ImGui::PopStyleVar();
        if (!isVisible) {
            mgr->SetVisible(WindowID::Welcome, false);
        }
    }

    std::function<void()> GetRenderFunction(WindowID id, gui::GuiManager* guiMgr) {
        switch (id) {
            case WindowID::DDEStatus:
                return [guiMgr]() { RenderDDEStatusWindow(guiMgr); };
            case WindowID::SystemInfo:
                return [guiMgr]() { RenderSystemInfoWindow(guiMgr); };
            case WindowID::SurfaceProfileInspector:
                return [guiMgr]() { RenderSurfaceProfileInspectorWindow(guiMgr); };
            case WindowID::SurfaceIrregularityMap:
                return [guiMgr]() { RenderSurfaceIrregularityMapWindow(guiMgr); };
            case WindowID::Logs:
                return [guiMgr]() { RenderLogsWindow(guiMgr); };
            case WindowID::Welcome:
                return [guiMgr]() { RenderWelcomeWindow(guiMgr); };
            default:
                return nullptr;
        }
    }
}

void DockableWindowsManager::RegisterDockableWindows(gui::GuiManager* guiMgr) {
    for (const auto& w : DockableWindows) {
        auto renderFn = GetRenderFunction(w.id, guiMgr);
        RegisterWindow(w.id, w.name, w.category, std::move(renderFn), w.order, w.isVisible);
    }
}