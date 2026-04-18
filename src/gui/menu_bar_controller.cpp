#include "menu_bar_controller.h"
#include "app/app.h"
#include "logger/logger.h"
#include "dde/dde_connection_manager.h"
#include "gui/constants.h"
#include "lib/imgui/imgui.h"
#include <format>

namespace gui {
    MenuBarController::MenuBarController(Logger& logger, DdeConnectionManager* ddeMgr) : m_logger(logger), m_pDdeMgr(ddeMgr) {}

    void MenuBarController::setExitCallback(std::function<void()> cb) {
        m_onExit = std::move(cb);
    }

    void MenuBarController::setAboutCallback(std::function<void()> cb) {
        m_onAbout = std::move(cb);
    }

    void MenuBarController::setDdeConnectionManager(DdeConnectionManager* ddeMgr) {
        m_pDdeMgr = ddeMgr;
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
            // Stage 4.1: DDE controls integrated into Menu
            if (ImGui::BeginMenu("DDE")) {
                if (m_pDdeMgr) {
                    if (ImGui::MenuItem("Connect to Zemax")) {
                        m_pDdeMgr->connect();
                    }
                    if (ImGui::MenuItem("Disconnect from Zemax")) {
                        m_pDdeMgr->disconnect();
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Info")) {
                if (ImGui::MenuItem("About")) {
                    if (m_onAbout) m_onAbout();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }
}
