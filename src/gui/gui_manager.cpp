#include "gui/gui.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "gui/surface_profile_service.h"
#include "gui/surface_irregularity_map_service.h"
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
    m_profileService = std::make_unique<SurfaceProfileService>(m_ddeConnectionManager, logger);
    m_irregularityMapService = std::make_unique<SurfaceIrregularityMapService>(m_ddeConnectionManager, logger);
    m_uiOpMonitor.setMonitor(m_zemaxDDEClient ? m_zemaxDDEClient->getOperationMonitor() : nullptr);
    m_profileService->setUiOperationMonitor(&m_uiOpMonitor);
    m_irregularityMapService->setUiOperationMonitor(&m_uiOpMonitor);
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

void GuiManager::initialize(bool isLightTheme, float dpiScale) {
    m_graphics.initialize(m_glfwWindow, m_logger, isLightTheme, dpiScale);
    m_menuBarController->setThemeToggleCallback([this]() {
        toggleTheme();
    });
}

void GuiManager::toggleTheme() {
    m_graphics.toggleTheme();
    m_logger.addLog(std::format("[GUI] Theme switched to: {}", m_graphics.getThemeManager().currentThemeName()));
}

void GuiManager::render() {
    // Refresh active DDE client in case connection changed via DDE Status UI
    m_zemaxDDEClient = m_ddeConnectionManager ? m_ddeConnectionManager->getActiveClient() : nullptr;
    m_uiOpMonitor.setMonitor(m_zemaxDDEClient ? m_zemaxDDEClient->getOperationMonitor() : nullptr);

    m_graphics.beginFrame();
    if (m_menuBarController) {
        m_menuBarController->render();
    }

    float navbarHeight = ImGui::GetFrameHeight();
    float statusBarHeight = m_uiOpMonitor.hasActiveTasks() ? ImGui::GetFrameHeight() * 1.5f : 0.0f;
    ImGui::SetNextWindowPos(ImVec2(0.0f, navbarHeight));
    ImGui::SetNextWindowSize(ImVec2(
        ImGui::GetIO().DisplaySize.x,
        ImGui::GetIO().DisplaySize.y - navbarHeight - statusBarHeight
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
        constexpr ImVec2 kDetachedWindowSize(600, 400);
        auto& tolSurface = m_profileService->m_tolerancedSurfaceData;
        auto& nomSurface = m_profileService->m_nominalSurfaceData;

        if (m_profileService->m_showTolerancedProfileWindow) {
            if (tolSurface.isValid()) {
                ImGui::SetNextWindowSize(kDetachedWindowSize, ImGuiCond_Once);
                std::string title = std::format("Toleranced Surface Profile ({}°, {} pts)", tolSurface.angle, tolSurface.sampling);
                if (ImGui::Begin(title.c_str(), &m_profileService->m_showTolerancedProfileWindow)) {
                    m_profileService->renderSurfaceProfilePlot("Toleranced", tolSurface, ImVec2(-1, -1));
                }
                ImGui::End();
            }
        }

        if (m_profileService->m_showNominalProfileWindow) {
            if (nomSurface.isValid()) {
                ImGui::SetNextWindowSize(kDetachedWindowSize, ImGuiCond_Once);
                std::string title = std::format("Nominal Surface Profile ({}°, {} pts)", nomSurface.angle, nomSurface.sampling);
                if (ImGui::Begin(title.c_str(), &m_profileService->m_showNominalProfileWindow)) {
                    m_profileService->renderSurfaceProfilePlot("Nominal", nomSurface, ImVec2(-1, -1));
                }
                ImGui::End();
            }
        }

        if (m_profileService->m_showComparisonProfileWindow) {
            if (nomSurface.isValid() && tolSurface.isValid()) {
                ImGui::SetNextWindowSize(kDetachedWindowSize, ImGuiCond_Once);
                if (ImGui::Begin("Surface Profile Comparison", &m_profileService->m_showComparisonProfileWindow)) {
                    m_profileService->renderProfileComparisonPlot("##DetachedProfiles", nomSurface, tolSurface, ImVec2(-1, -1));
                }
                ImGui::End();
            }
        }

        if (m_profileService->m_showDeviationProfileWindow) {
            if (nomSurface.isValid() && tolSurface.isValid()) {
                ImGui::SetNextWindowSize(kDetachedWindowSize, ImGuiCond_Once);
                if (ImGui::Begin("Surface Profile Irregularity (PV)", &m_profileService->m_showDeviationProfileWindow)) {
                    m_profileService->renderProfileDeviationPlot("##DetachedDeviation", nomSurface, tolSurface, ImVec2(-1, -1));
                }
                ImGui::End();
            }
        }
    }

    {
        if (m_irregularityMapService->m_showTolerancedSurfaceMap) {
            if (m_irregularityMapService->hasData()) {
                ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);
                if (ImGui::Begin("Surface Irregularity Map 3D", &m_irregularityMapService->m_showTolerancedSurfaceMap)) {
                    m_irregularityMapService->renderSurfaceMapPlot(ImVec2(-1, -1));
                }
                ImGui::End();
            } else {
                m_irregularityMapService->m_showTolerancedSurfaceMap = false;
            }
        }

        if (m_irregularityMapService->m_showDeviationSurfaceMap) {
            if (m_irregularityMapService->hasData() && m_irregularityMapService->m_nominalSurfaceData.isValid()) {
                ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);
                if (ImGui::Begin("Surface Irregularity Map 3D Deviation", &m_irregularityMapService->m_showDeviationSurfaceMap)) {
                    m_irregularityMapService->renderDeviationSurfaceMapPlot(ImVec2(-1, -1));
                }
                ImGui::End();
            } else {
                m_irregularityMapService->m_showDeviationSurfaceMap = false;
            }
        }

        if (m_irregularityMapService->m_showWorstSectionProfile) {
            if (m_irregularityMapService->m_worstProfileData.isValid()) {
                constexpr ImVec2 kDetachedWndSize(600, 400);
                ImGui::SetNextWindowSize(kDetachedWndSize, ImGuiCond_Once);
                if (ImGui::Begin("Worst Section Profile", &m_irregularityMapService->m_showWorstSectionProfile)) {
                    auto& wp = m_irregularityMapService->m_worstProfileData;
                    std::string title = std::format("Worst Section ({}°, {} pts)", wp.angle, wp.sampling);
                    m_profileService->renderSurfaceProfilePlot(title.c_str(), wp, ImVec2(-1, -1));
                }
                ImGui::End();
            } else {
                m_irregularityMapService->m_showWorstSectionProfile = false;
            }
        }

        if (m_irregularityMapService->m_showWorstSectionDeviation) {
            if (m_irregularityMapService->m_worstProfileData.isValid()
                && m_irregularityMapService->m_nominalSurfaceData.isValid()) {
                constexpr ImVec2 kDetachedWndSize(600, 400);
                ImGui::SetNextWindowSize(kDetachedWndSize, ImGuiCond_Once);
                if (ImGui::Begin("Worst Section Deviation", &m_irregularityMapService->m_showWorstSectionDeviation)) {
                    m_profileService->renderProfileDeviationPlot("##WorstDeviation",
                        m_irregularityMapService->m_nominalSurfaceData,
                        m_irregularityMapService->m_worstProfileData, ImVec2(-1, -1));
                }
                ImGui::End();
            } else {
                m_irregularityMapService->m_showWorstSectionDeviation = false;
            }
        }
    }

    ImGuiUtils::SetPopupWindowPosition();
    renderUpdatesPopup();
    renderAboutPopup();

    m_uiOpMonitor.renderGlobalStatusBar();

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

