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
}

void DockableWindowsManager::SaveState() {
    nlohmann::json j;
    for (const auto& w : windows_) {
        j[w.name] = w.isVisible;
    }
    std::ofstream out(app::getWindowStatePath());
    out << j.dump(4);
}

std::vector<WindowID> DockableWindowsManager::GetIDsByCategory(WindowCategory category) const {
    std::vector<WindowID> result;
    for (const auto& w : windows_) {
        if (w.category == category) {
            result.push_back(w.id);
        }
    }
    std::stable_sort(result.begin(), result.end(), [this](WindowID a, WindowID b) {
        int orderA = 0, orderB = 0;
        for (const auto& w : windows_) {
            if (w.id == a) orderA = w.order;
            if (w.id == b) orderB = w.order;
        }
        return orderA < orderB;
    });
    return result;
}

const std::vector<std::pair<WindowID, bool>> DockableWindowsManager::GetVisibilities() const {
    std::vector<std::pair<WindowID, bool>> result;
    for (const auto& w : windows_) {
        result.push_back({w.id, w.isVisible});
    }
    return result;
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

    void RenderDebugLogWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        auto* mgr = guiMgr->getWindowManager();
        bool isVisible = mgr->IsVisible(WindowID::DebugLog);
        const char* title = mgr->GetName(WindowID::DebugLog);
        ImGuiUtils::SetDpiScaledWindowConstraints(gui::DEBUG_LOG_WINDOW_MIN_SIZE.x, gui::DEBUG_LOG_WINDOW_MIN_SIZE.y);
        if (ImGui::Begin(title, &isVisible)) {
            guiMgr->renderDebugLog();
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::DebugLog, false);
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
            case WindowID::DebugLog:
                return [guiMgr]() { RenderDebugLogWindow(guiMgr); };
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