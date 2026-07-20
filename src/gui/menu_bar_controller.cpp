#include "menu_bar_controller.h"
#include "app/app.h"
#include "logger/logger.h"
#include "dde/dde_connection_manager.h"
#include "gui/constants.h"
#include "gui/dockable_windows_manager.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
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

    void MenuBarController::setWindowManager(DockableWindowsManager* wndMgr) {
        m_pWndMgr = wndMgr;
    }

    void MenuBarController::setPreferencesCallback(std::function<void()> cb) {
        m_onPreferences = std::move(cb);
    }

    void MenuBarController::openPreferences() {
        if (m_onPreferences) m_onPreferences();
    }

    void MenuBarController::render() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu(ICON_FA_FOLDER_OPEN " File")) {
                if (ImGui::MenuItem(ICON_FA_FILE_IMPORT " Open *.ZMX file in Zemax", "Ctrl+O")) App::openZmxFileInZemax(m_logger);
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_FA_GEAR " Preferences", "Ctrl+,", false, true)) {
                    if (m_onPreferences) m_onPreferences();
                }
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_FA_RIGHT_FROM_BRACKET " Exit", "Alt+F4")) {
                    if (m_onExit) m_onExit();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(ICON_FA_COMPUTER " DDE")) {
                if (m_pWndMgr) {
                    bool showDDEStatus = m_pWndMgr->IsVisible(WindowID::DDEStatus);
                    if (ImGui::MenuItem("Show DDE Status", nullptr, &showDDEStatus)) {
                        m_pWndMgr->SetVisible(WindowID::DDEStatus, showDDEStatus);
                    }
                }
                ImGui::EndMenu();
            }
            if (m_pWndMgr && ImGui::BeginMenu(ICON_FA_WRENCH " Tools")) {
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
            if (ImGui::BeginMenu(ICON_FA_CLIPBOARD_LIST " Info")) {
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
