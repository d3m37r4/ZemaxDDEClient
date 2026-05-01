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
    m_debugLogViewer    = std::make_unique<DebugLogViewer>();
    m_appInfoDialog     = std::make_unique<AppInfoDialog>();
    if (m_sidebarRenderer) {
        m_sidebarRenderer->setPageSwitcher([this](GuiPage page){ m_currentPage = page; });
    }
}

GuiManager::~GuiManager() = default;

void GuiManager::initialize(float dpiScale) {
    const char* iniPath = "imgui.ini";
    m_graphics.initialize(m_glfwWindow, m_logger, iniPath, dpiScale);
}

void GuiManager::render() {
    m_graphics.beginFrame();
    if (m_menuBarController) {
        m_menuBarController->render();
    }

    // Create main docking space (restored from container.cpp)
    float navbarHeight = ImGui::GetFrameHeight();
    ImGui::SetNextWindowPos(ImVec2(0.0f, navbarHeight));
    ImGui::SetNextWindowSize(ImVec2(
        ImGui::GetIO().DisplaySize.x,
        ImGui::GetIO().DisplaySize.y - navbarHeight
    ));
    ImGui::Begin("MainDockSpace", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus
    );
    ImGuiID dockSpaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();

    renderSidebar();
    renderContent();

    // Render debug log window
    renderDebugLog();

    // Sag analysis windows (delegated to service)
    if (m_sagService->m_showTolerancedSagWindow) {
        auto& surface = m_zemaxDDEClient->getTolerancedSurface();
        if (surface.isValid()) {
            std::string title = std::format("Toleranced Surface Cross Section ({}°, {} pts)", surface.angle, surface.sampling);
            m_sagService->renderCrossSectionWindow(title.c_str(), "Toleranced", surface, &m_sagService->m_showTolerancedSagWindow);
        }
    }

    if (m_sagService->m_showNominalSagWindow) {
        auto& surface = m_zemaxDDEClient->getNominalSurface();
        if (surface.isValid()) {
            std::string title = std::format("Nominal Surface Cross Section ({}°, {} pts)", surface.angle, surface.sampling);
            m_sagService->renderCrossSectionWindow(title.c_str(), "Nominal", surface, &m_sagService->m_showNominalSagWindow);
        }
    }

    if (m_sagService->m_showComparisonWindow) {
        auto& nom = m_zemaxDDEClient->getNominalSurface();
        auto& tol = m_zemaxDDEClient->getTolerancedSurface();
        if (nom.isValid() && tol.isValid()) {
            m_sagService->renderComparisonWindow(nom, tol, &m_sagService->m_showComparisonWindow);
        }
    }

    if (m_sagService->m_showErrorWindow) {
        auto& nom = m_zemaxDDEClient->getNominalSurface();
        auto& tol = m_zemaxDDEClient->getTolerancedSurface();
        if (nom.isValid() && tol.isValid() && nom.sagDataPoints.size() == tol.sagDataPoints.size()) {
            m_sagService->renderErrorWindow(nom, tol, &m_sagService->m_showErrorWindow);
        }
    }

    // Render popups
    setPopupPosition();
    renderUpdatesPopup();
    renderAboutPopup();

    m_graphics.endFrame();
}

void GuiManager::renderSidebar() {
    if (m_sidebarRenderer) {
        m_sidebarRenderer->renderSidebar(m_zemaxDDEClient, m_logger);
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

void GuiManager::renderDebugLog() {
    if (m_debugLogViewer) {
        m_debugLogViewer->render(m_logger);
    }
}

void GuiManager::setPopupPosition() {
    if (m_appInfoDialog) {
        m_appInfoDialog->setPopupPosition();
    }
}

void GuiManager::renderAboutPopup() {
    if (m_appInfoDialog) {
        m_appInfoDialog->render(m_showAboutPopup);
    }
}

void GuiManager::renderUpdatesPopup() {
    if (m_appInfoDialog) {
        m_appInfoDialog->renderUpdatesPopup(m_showUpdatesPopup);
    }
}

} // namespace gui
