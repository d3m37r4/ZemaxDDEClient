#include "gui/gui.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "gui/sag_analysis_service.h"
#include "gui/sag_map_analysis_service.h"
#include "gui/menu_bar_controller.h"
#include "gui/imgui_utils.h"
#include "gui/dockable_windows_manager.h"
#include "dde/dde_connection_manager.h"
#include "windows_dockable/dde_status.h"
#include "logger/logger.h"

namespace gui {
    GuiManager::GuiManager(GLFWwindow* glfwWindow, DDEConnectionManager* ddeConnectionManager, Logger& logger)
: m_glfwWindow(glfwWindow)
, m_ddeConnectionManager(ddeConnectionManager)
, m_zemaxDDEClient(ddeConnectionManager ? ddeConnectionManager->getActiveClient() : nullptr)
, m_logger(logger)
{
    m_sagService = std::make_unique<SagAnalysisService>(m_ddeConnectionManager, logger);
    m_sagMapService = std::make_unique<SagMapAnalysisService>(m_ddeConnectionManager, logger);
    m_menuBarController = std::make_unique<MenuBarController>(m_logger, m_ddeConnectionManager);
    m_menuBarController->setExitCallback([this]() {
        if (m_glfwWindow) glfwSetWindowShouldClose(m_glfwWindow, true);
    });
    m_menuBarController->setAboutCallback([this]() {
        m_showAboutPopup = true;
    });
    m_menuBarController->setUpdatesCallback([this]() {
        m_showUpdatesPopup = true;
    });
    m_ddeStatusRenderer = std::make_unique<DDEStatus>(m_ddeConnectionManager);
    m_debugLogRenderer = std::make_unique<DebugLog>();
    m_aboutDialog        = std::make_unique<AboutDialog>();
    m_updateChecker      = std::make_unique<UpdateChecker>();
}

GuiManager::~GuiManager() = default;

void GuiManager::initialize(float dpiScale) {
    m_graphics.initialize(m_glfwWindow, m_logger, dpiScale);
}

void GuiManager::render() {
    // Refresh active DDE client in case connection changed via DDE Status UI
    m_zemaxDDEClient = m_ddeConnectionManager ? m_ddeConnectionManager->getActiveClient() : nullptr;

    m_graphics.beginFrame();
    if (m_menuBarController) {
        m_menuBarController->render();
    }

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
    ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_NoCloseButton);
    ImGui::End();

    if (m_pWndMgr) {
        m_pWndMgr->RenderAll();
    }

    {
        auto& tolSurface = m_sagService->m_tolerancedSurfaceData;
        auto& nomSurface = m_sagService->m_nominalSurfaceData;

        if (m_sagService->m_showTolerancedProfileWindow) {
            if (tolSurface.isValid()) {
                ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
                std::string title = std::format("Toleranced Surface Profile ({}°, {} pts)", tolSurface.angle, tolSurface.sampling);
                if (ImGui::Begin(title.c_str(), &m_sagService->m_showTolerancedProfileWindow)) {
                    m_sagService->renderSurfaceProfilePlot("Toleranced", tolSurface, ImVec2(-1, -1));
                }
                ImGui::End();
            }
        }

        if (m_sagService->m_showNominalProfileWindow) {
            if (nomSurface.isValid()) {
                ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
                std::string title = std::format("Nominal Surface Profile ({}°, {} pts)", nomSurface.angle, nomSurface.sampling);
                if (ImGui::Begin(title.c_str(), &m_sagService->m_showNominalProfileWindow)) {
                    m_sagService->renderSurfaceProfilePlot("Nominal", nomSurface, ImVec2(-1, -1));
                }
                ImGui::End();
            }
        }

        if (m_sagService->m_showComparisonProfileWindow) {
            if (nomSurface.isValid() && tolSurface.isValid()) {
                ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
                if (ImGui::Begin("Surface Profile Comparison", &m_sagService->m_showComparisonProfileWindow)) {
                    m_sagService->renderProfileComparisonPlot("##DetachedProfiles", nomSurface, tolSurface, ImVec2(-1, -1));
                }
                ImGui::End();
            }
        }

        if (m_sagService->m_showDeviationProfileWindow) {
            if (nomSurface.isValid() && tolSurface.isValid()) {
                ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
                if (ImGui::Begin("Surface Profile Irregularity (PV)", &m_sagService->m_showDeviationProfileWindow)) {
                    m_sagService->renderProfileDeviationPlot("##DetachedDeviation", nomSurface, tolSurface, ImVec2(-1, -1));
                }
                ImGui::End();
            }
        }
    }

    ImGuiUtils::SetPopupWindowPosition();
    renderUpdatesPopup();
    renderAboutPopup();

    m_graphics.endFrame();
}

void GuiManager::updateDpiStyle(float dpiScale) {
    m_graphics.updateDpiStyle(dpiScale);
}

void GuiManager::renderDebugLog() {
    if (m_debugLogRenderer) {
        m_debugLogRenderer->render(m_logger);
    }
}

void GuiManager::renderAboutPopup() {
    if (m_aboutDialog) {
        m_aboutDialog->render(m_showAboutPopup);
    }
}

void GuiManager::renderUpdatesPopup() {
    if (m_updateChecker) {
        m_updateChecker->renderPopup(m_showUpdatesPopup);
    }
}

}
