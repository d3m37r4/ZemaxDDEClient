#include "menu_bar_controller.h"
#include "app/app.h"
#include "logger/logger.h"
#include "dde/dde_connection_manager.h"
#include "gui/constants.h"
#include "gui/dockable_windows_manager.h"
#include "lib/imgui/imgui.h"
#include <format>

namespace gui {
    MenuBarController::MenuBarController(Logger& logger, DDEConnectionManager* ddeMgr) : m_logger(logger), m_pDDEClientMgr(ddeMgr) {}

    void MenuBarController::setExitCallback(std::function<void()> cb) {
        m_onExit = std::move(cb);
    }

    void MenuBarController::setAboutCallback(std::function<void()> cb) {
        m_onAbout = std::move(cb);
    }

    void MenuBarController::setUpdatesCallback(std::function<void()> cb) {
        m_onUpdates = std::move(cb);
    }

    void MenuBarController::setDDEConnectionManager(DDEConnectionManager* ddeMgr) {
        m_pDDEClientMgr = ddeMgr;
    }

    void MenuBarController::setWindowManager(DockableWindowsManager* wndMgr) {
        m_pWndMgr = wndMgr;
    }

    void MenuBarController::setSidebarToggleCallback(std::function<void(bool)> cb) {
        m_onSidebarToggle = std::move(cb);
    }

    void MenuBarController::render() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Menu")) {
                if (ImGui::MenuItem("Open *.ZMX file in Zemax", "Ctrl+O")) App::openZmxFileInZemax(m_logger);
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    if (m_onExit) m_onExit();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("DDE")) {
                if (m_pWndMgr) {
                    bool showDDEStatus = m_pWndMgr->IsVisible(WindowID::DDEStatus);
                    if (ImGui::MenuItem("Show DDE Status", nullptr, &showDDEStatus)) {
                        m_pWndMgr->SetVisible(WindowID::DDEStatus, showDDEStatus);
                    }
                }
                ImGui::EndMenu();
            }
            if (m_pWndMgr && ImGui::BeginMenu("Tools")) {
                auto ids = m_pWndMgr->GetIDsByCategory(WindowCategory::Tools);
                for (WindowID id : ids) {
                    bool visible = m_pWndMgr->IsVisible(id);
                    const char* name = m_pWndMgr->GetName(id);
                    if (!name) continue;
                    if (ImGui::MenuItem(name, nullptr, &visible)) {
                        m_pWndMgr->SetVisible(id, visible);
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Info")) {
                if (m_pWndMgr) {
                    auto ids = m_pWndMgr->GetIDsByCategory(WindowCategory::Info);
                    for (WindowID id : ids) {
                        bool visible = m_pWndMgr->IsVisible(id);
                        const char* name = m_pWndMgr->GetName(id);
                        if (!name) continue;
                        if (ImGui::MenuItem(name, nullptr, &visible)) {
                            m_pWndMgr->SetVisible(id, visible);
                        }
                    }
                    if (!ids.empty()) {
                        ImGui::Separator();
                    }
                }
                if (ImGui::MenuItem("Check for Updates")) {
                    if (m_onUpdates) m_onUpdates();
                }
                if (ImGui::MenuItem("About")) {
                    if (m_onAbout) m_onAbout();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }
}
