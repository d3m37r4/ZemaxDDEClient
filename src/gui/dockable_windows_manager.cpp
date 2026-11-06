#include "dockable_windows_manager.h"
#include "gui.h"
#include "gui/utils.h"
#include "gui/imgui_utils.h"
#include "gui/window_chrome.h"
#include "app/config_path.h"
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace {
// Chrome state per panel is keyed by the WindowID value itself: the local
// `isVisible` bool seen by the render helpers lives on the stack, its
// address changes every frame and cannot key persistent state.
inline const void* chromeToken(WindowID id) {
    return reinterpret_cast<const void*>(static_cast<std::uintptr_t>(id));
}

// The same identity as a string, for the restart-persistent chrome_state.json
// (paired with chromeToken() at every chrome call site for the same window).
inline const char* chromePersistKey(WindowID id) {
    switch (id) {
        case WindowID::DDEStatus: return "panel:DDEStatus";
        case WindowID::SystemInfo: return "panel:SystemInfo";
        case WindowID::SurfaceProfileInspector: return "panel:SurfaceProfileInspector";
        case WindowID::SurfaceIrregularityMap: return "panel:SurfaceIrregularityMap";
        case WindowID::Logs: return "panel:Logs";
        default: return nullptr;
    }
}
namespace chrome = gui::chrome;
}

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
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    LoadStateFromString(data);
}

void DockableWindowsManager::LoadStateFromString(std::string_view json) {
    if (json.empty()) return;
    try {
        const nlohmann::json j = nlohmann::json::parse(json.begin(), json.end());
        if (!j.is_object()) return;
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
    std::ofstream out(app::getWindowStatePath());
    out << SaveStateToString();
}

std::string DockableWindowsManager::SaveStateToString() const {
    nlohmann::json j;
    for (const auto& w : windows_) {
        j[w.name] = w.isVisible;
    }
    return j.dump(4);
}

void DockableWindowsManager::ResetVisibilityToDefaults() {
    for (auto& w : windows_) {
        for (const auto& def : DockableWindows) {
            if (def.id == w.id) {
                w.isVisible = def.isVisible;
                break;
            }
        }
    }
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
        chrome::setPersistentKey(
            chromeToken(WindowID::DDEStatus), chromePersistKey(WindowID::DDEStatus));
        // Tile x closes the panel like the View-menu toggle.
        chrome::setCloseHandler(chromeToken(WindowID::DDEStatus),
            [mgr] { mgr->SetVisible(WindowID::DDEStatus, false); });
        if (chrome::minimizedAndShelved(chromeToken(WindowID::DDEStatus), mgr->GetName(WindowID::DDEStatus))) return;
        bool isVisible = mgr->IsVisible(WindowID::DDEStatus);
        const char* title = mgr->GetName(WindowID::DDEStatus);
        ImGuiUtils::SetChromeWindowConstraints(gui::DDE_STATUS_WINDOW_MIN_SIZE.x, gui::DDE_STATUS_WINDOW_MIN_SIZE.y);
        chrome::applyFrameRect(chromeToken(WindowID::DDEStatus));
        if (ImGui::Begin(title, &isVisible,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            ImGuiUtils::ClampWindowToViewport(chrome::isMaximized(chromeToken(WindowID::DDEStatus)));
            chrome::drawTitleBarButtons(chromeToken(WindowID::DDEStatus), &isVisible);
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
        chrome::setPersistentKey(
            chromeToken(WindowID::SystemInfo), chromePersistKey(WindowID::SystemInfo));
        chrome::setCloseHandler(chromeToken(WindowID::SystemInfo),
            [mgr] { mgr->SetVisible(WindowID::SystemInfo, false); });
        if (chrome::minimizedAndShelved(chromeToken(WindowID::SystemInfo), mgr->GetName(WindowID::SystemInfo))) return;
        bool isVisible = mgr->IsVisible(WindowID::SystemInfo);
        const char* title = mgr->GetName(WindowID::SystemInfo);
        ImGuiUtils::SetChromeWindowConstraints(gui::OPTICAL_SYSTEM_INFO_WINDOW_MIN_SIZE.x, gui::OPTICAL_SYSTEM_INFO_WINDOW_MIN_SIZE.y);
        chrome::applyFrameRect(chromeToken(WindowID::SystemInfo));
        if (ImGui::Begin(title, &isVisible)) {
            ImGuiUtils::ClampWindowToViewport(chrome::isMaximized(chromeToken(WindowID::SystemInfo)));
            chrome::drawTitleBarButtons(chromeToken(WindowID::SystemInfo), &isVisible);
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
        chrome::setPersistentKey(chromeToken(WindowID::SurfaceProfileInspector),
            chromePersistKey(WindowID::SurfaceProfileInspector));
        chrome::setCloseHandler(chromeToken(WindowID::SurfaceProfileInspector),
            [mgr] { mgr->SetVisible(WindowID::SurfaceProfileInspector, false); });
        if (chrome::minimizedAndShelved(chromeToken(WindowID::SurfaceProfileInspector), mgr->GetName(WindowID::SurfaceProfileInspector))) return;
        bool isVisible = mgr->IsVisible(WindowID::SurfaceProfileInspector);
        const char* title = mgr->GetName(WindowID::SurfaceProfileInspector);
        ImGuiUtils::SetChromeWindowConstraints(gui::SURFACE_PROFILE_INSPECTOR_WINDOW_MIN_SIZE.x, gui::SURFACE_PROFILE_INSPECTOR_WINDOW_MIN_SIZE.y);
        chrome::applyFrameRect(chromeToken(WindowID::SurfaceProfileInspector));
        if (ImGui::Begin(title, &isVisible)) {
            ImGuiUtils::ClampWindowToViewport(chrome::isMaximized(chromeToken(WindowID::SurfaceProfileInspector)));
            chrome::drawTitleBarButtons(chromeToken(WindowID::SurfaceProfileInspector), &isVisible);
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
        chrome::setPersistentKey(chromeToken(WindowID::SurfaceIrregularityMap),
            chromePersistKey(WindowID::SurfaceIrregularityMap));
        chrome::setCloseHandler(chromeToken(WindowID::SurfaceIrregularityMap),
            [mgr] { mgr->SetVisible(WindowID::SurfaceIrregularityMap, false); });
        if (chrome::minimizedAndShelved(chromeToken(WindowID::SurfaceIrregularityMap), mgr->GetName(WindowID::SurfaceIrregularityMap))) return;
        bool isVisible = mgr->IsVisible(WindowID::SurfaceIrregularityMap);
        const char* title = mgr->GetName(WindowID::SurfaceIrregularityMap);
        ImGuiUtils::SetChromeWindowConstraints(gui::SURFACE_IRREGULARITY_MAP_WINDOW_MIN_SIZE.x, gui::SURFACE_IRREGULARITY_MAP_WINDOW_MIN_SIZE.y);
        chrome::applyFrameRect(chromeToken(WindowID::SurfaceIrregularityMap));
        if (ImGui::Begin(title, &isVisible)) {
            ImGuiUtils::ClampWindowToViewport(chrome::isMaximized(chromeToken(WindowID::SurfaceIrregularityMap)));
            chrome::drawTitleBarButtons(chromeToken(WindowID::SurfaceIrregularityMap), &isVisible);
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
        chrome::setPersistentKey(chromeToken(WindowID::Logs), chromePersistKey(WindowID::Logs));
        chrome::setCloseHandler(chromeToken(WindowID::Logs),
            [mgr] { mgr->SetVisible(WindowID::Logs, false); });
        if (chrome::minimizedAndShelved(chromeToken(WindowID::Logs), mgr->GetName(WindowID::Logs))) return;
        bool isVisible = mgr->IsVisible(WindowID::Logs);
        const char* title = mgr->GetName(WindowID::Logs);
        ImGuiUtils::SetChromeWindowConstraints(gui::LOGS_WINDOW_MIN_SIZE.x, gui::LOGS_WINDOW_MIN_SIZE.y);
        chrome::applyFrameRect(chromeToken(WindowID::Logs));
        if (ImGui::Begin(title, &isVisible)) {
            ImGuiUtils::ClampWindowToViewport(chrome::isMaximized(chromeToken(WindowID::Logs)));
            chrome::drawTitleBarButtons(chromeToken(WindowID::Logs), &isVisible);
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
        // The card is only meaningful while the DDE link is down.
        if (guiMgr->isDDEInitialized()) {
            if (mgr->IsVisible(WindowID::Welcome)) {
                mgr->SetVisible(WindowID::Welcome, false);
            }
            return;
        }
        // Dock area rect: the centering target and the clamp reference.
        // Multi-viewport: window coords are desktop coords, so the dock area
        // rect must be anchored to the main viewport's own position, not to
        // a (0,0) origin which now points at the desktop corner.
        const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        const float dockTop = ImGui::GetFrameHeight();
        const ImVec2 dockAreaMin(mainViewport->Pos.x,
                                 mainViewport->Pos.y + dockTop);
        const ImVec2 dockAreaMax(mainViewport->Pos.x + mainViewport->Size.x,
                                 mainViewport->Pos.y + mainViewport->Size.y -
                                 guiMgr->computeStatusBarHeight());
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