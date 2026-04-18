#include "gui/gui.h"
#include <imgui.h>
// SagAnalysisService header must be visible for construction of m_sagService
#include "gui/sag_analysis_service.h"
// Stage 2: infrastructure delegates
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
    // Stage 1: instantiate infrastructure components
    m_sagService = std::make_unique<SagAnalysisService>(ddeClient, logger);
    m_sagController = std::make_unique<SagAnalysisController>();
    // Stage 4: create Sag controller and DDE manager for AppUI
    // Initialize lightweight infrastructure components for Stage 2
    // Stage 4.1: create DdeConnectionManager and wire to MenuBarController
    m_ddeConnectionManager = std::make_unique<DdeConnectionManager>(m_zemaxDDEClient, m_logger);
    m_menuBarController = std::make_unique<MenuBarController>(m_logger, m_ddeConnectionManager.get());
    // Wire callbacks to GuiManager state
    m_menuBarController->setExitCallback([this]() {
        if (m_glfwWindow) glfwSetWindowShouldClose(m_glfwWindow, true);
    });
    m_menuBarController->setAboutCallback([this]() {
        m_showAboutPopup = true;
    });
    m_sidebarRenderer   = std::make_unique<SidebarRenderer>();
    m_contentRouter     = std::make_unique<ContentRouter>();
    // Wire stage-3 page switch callback from SidebarRenderer to GuiManager current page
    if (m_sidebarRenderer) {
        m_sidebarRenderer->setPageSwitcher([this](GuiPage page){ m_currentPage = page; });
    }
    }

GuiManager::~GuiManager() = default;

void GuiManager::initialize() {
    // Stage 3: initialize ImGui back-end (graphics) to enable rendering
    // Use a simple INI path; full Windows path handling can be reintroduced later
    const char* iniPath = "imgui.ini";
    m_graphics.initialize(m_glfwWindow, m_logger, iniPath);
}

void GuiManager::render() {
    m_graphics.beginFrame();
    // Stage 3: Navbar handled by MenuBarController
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
