#include "gui/gui.h"
#include <imgui.h>
#include "gui/sag_analysis_service.h"
#include "gui/menu_bar_controller.h"
#include "dde/dde_connection_manager.h"
#include "gui/sidebar_renderer.h"
#include "gui/content_router.h"
#include "gui/sag_analysis_controller.h"

namespace gui {

    GuiManager::GuiManager(GLFWwindow* glfwWindow, HWND hwndClient, ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger)
: m_glfwWindow(glfwWindow)
, m_hwndClient(hwndClient)
, m_zemaxDDEClient(ddeClient)
, m_logger(logger)
{
    m_sagService = std::make_unique<SagAnalysisService>(ddeClient, logger);
    m_sagController = std::make_unique<SagAnalysisController>();
    m_ddeConnectionManager = std::make_unique<DdeConnectionManager>(m_zemaxDDEClient, m_logger);
    m_menuBarController = std::make_unique<MenuBarController>(m_logger, m_ddeConnectionManager.get());
    m_menuBarController->setExitCallback([this]() {
        if (m_glfwWindow) glfwSetWindowShouldClose(m_glfwWindow, true);
    });
    m_menuBarController->setAboutCallback([this]() {
        m_showAboutPopup = true;
    });
    m_sidebarRenderer   = std::make_unique<SidebarRenderer>();
    m_contentRouter     = std::make_unique<ContentRouter>();
    if (m_sidebarRenderer) {
        m_sidebarRenderer->setPageSwitcher([this](GuiPage page){ m_currentPage = page; });
    }
}

GuiManager::~GuiManager() = default;

void GuiManager::initialize() {
    const char* iniPath = "imgui.ini";
    m_graphics.initialize(m_glfwWindow, m_logger, iniPath);
}

void GuiManager::render() {
    m_graphics.beginFrame();
    if (m_menuBarController) {
        m_menuBarController->render();
    }
    renderSidebar();
    renderContent();
    m_graphics.endFrame();
}

void GuiManager::renderSidebar() {
    if (m_sidebarRenderer) {
        m_sidebarRenderer->renderSidebar(m_zemaxDDEClient);
    }
}

void GuiManager::renderContent() {
    if (m_contentRouter) {
        m_contentRouter->renderContent(m_currentPage, this);
    }
}

void GuiManager::updateDpiStyle(float dpiScale) {
    m_graphics.updateDpiStyle(dpiScale);
}

} // namespace gui
