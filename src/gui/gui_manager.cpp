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

    // When a connection slot is torn down, drop tasks bound to it so the status
    // bar never dereferences a destroyed OperationMonitor (use-after-free guard).
    if (ddeConnectionManager) {
        ddeConnectionManager->setOnClientDisconnect([this](int index) {
            m_uiOpMonitor.failAllTasksOnSlot(index);
            m_profileService->onClientDisconnected(index);
            m_irregularityMapService->onClientDisconnected(index);
        });
    }
    m_menuBarController = std::make_unique<MenuBarController>(m_logger, m_ddeConnectionManager);
    m_menuBarController->setExitCallback([this]() {
        if (m_glfwWindow) glfwSetWindowShouldClose(m_glfwWindow, true);
    });
    m_menuBarController->setAboutCallback([this]() {
        m_aboutDialog->open();
    });
    m_menuBarController->setUpdatesCallback([this]() {
        m_updateChecker->open();
    });
    m_ddeStatusRenderer = std::make_unique<DDEStatus>(m_ddeConnectionManager);
    m_logsRenderer = std::make_unique<Logs>();
    m_aboutDialog        = std::make_unique<AboutDialog>();
    m_connectionLostDialog = std::make_unique<ConnectionLostDialog>();
    m_updateChecker      = std::make_unique<UpdateChecker>();
    m_settingsManager   = std::make_unique<SettingsManager>();
    m_preferencesDialog = std::make_unique<PreferencesDialog>(*m_settingsManager);
    m_preferencesDialog->setLogger(&m_logger);
    m_preferencesDialog->setThemeManager(&m_graphics.getThemeManager());
    m_settingsManager->setUpdateChecker(m_updateChecker.get());
    m_settingsManager->setLogger(&m_logger);
    m_menuBarController->setPreferencesCallback([this]() {
        m_preferencesDialog->setActiveLogPath(m_logger.getCurrentLogPath());
        m_preferencesDialog->open();
    });
}

GuiManager::~GuiManager() = default;

void GuiManager::initialize(bool isLightTheme, float dpiScale) {
    m_graphics.initialize(m_glfwWindow, m_logger, isLightTheme, dpiScale);
    m_settingsManager->bind(&m_graphics.getThemeManager(), m_ddeConnectionManager);
    m_settingsManager->setGraphicsBackend(&m_graphics);
    m_profileService->setSettingsManager(m_settingsManager.get());
    m_irregularityMapService->setSettingsManager(m_settingsManager.get());

    // Initialize 3D map colormaps from saved settings
    const auto& mapSettings = m_settingsManager->current().map;
    m_irregularityMapService->m_windowState.selectedColormapSurface = mapSettings.defaultColormapSurface;
    m_irregularityMapService->m_windowState.selectedColormapDeviation = mapSettings.defaultColormapDeviation;
    m_irregularityMapService->m_windowState.highlightWorstSurface = mapSettings.highlightWorstSurface;
    m_irregularityMapService->m_windowState.highlightWorstDeviation = mapSettings.highlightWorstDeviation;
    m_irregularityMapService->m_windowState.worstColorSurface[0] = mapSettings.worstColorSurface[0];
    m_irregularityMapService->m_windowState.worstColorSurface[1] = mapSettings.worstColorSurface[1];
    m_irregularityMapService->m_windowState.worstColorSurface[2] = mapSettings.worstColorSurface[2];
    m_irregularityMapService->m_windowState.worstColorDeviation[0] = mapSettings.worstColorDeviation[0];
    m_irregularityMapService->m_windowState.worstColorDeviation[1] = mapSettings.worstColorDeviation[1];
    m_irregularityMapService->m_windowState.worstColorDeviation[2] = mapSettings.worstColorDeviation[2];

    const auto& themeManager = m_graphics.getThemeManager();
    m_ddeStatusRenderer->setThemeManager(&themeManager);
    m_ddeStatusRenderer->setLogger(&m_logger);
    m_updateChecker->setThemeManager(&themeManager);
}

void GuiManager::render() {
    // Refresh active DDE client in case connection changed via DDE Status UI
    m_zemaxDDEClient = m_ddeConnectionManager ? m_ddeConnectionManager->getActiveClient() : nullptr;
    m_uiOpMonitor.setMonitor(m_zemaxDDEClient ? m_zemaxDDEClient->getOperationMonitor() : nullptr);

    // Process DDE timeouts and check connection health
    if (m_ddeConnectionManager) {
        m_ddeConnectionManager->processAllTimeouts();
        m_ddeConnectionManager->checkAllConnectionHealth();
    }

    // Check system theme change every 60 frames (once per second at 60 FPS)
    constexpr unsigned int kThemeCheckInterval = 60;
    if (++m_frameCount % kThemeCheckInterval == 0) {
        m_settingsManager->checkAndApplySystemTheme();
    }

    m_graphics.beginFrame();
    if (m_menuBarController) {
        m_menuBarController->render();
    }

    float navbarHeight = ImGui::GetFrameHeight();
    float statusBarHeight = m_uiOpMonitor.computeStatusBarHeight();
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

    const ImVec2 kDetachedWindowSize(600, 400);

    auto& profileUnits = m_profileService->m_windowState.units;
    auto& mapUnits = m_irregularityMapService->m_windowState.units;

    {
        auto& tolSurface = m_profileService->m_tolerancedSurfaceData;
        auto& nomSurface = m_profileService->m_nominalSurfaceData;

        if (m_profileService->m_showTolerancedProfileWindow) {
            if (tolSurface.isValid()) {
                ImGui::SetNextWindowSize(ImGuiUtils::DpiScaleVec2(kDetachedWindowSize), ImGuiCond_Once);
                std::string title = std::format("Toleranced Surface Profile ({}°, {} pts)", tolSurface.angle, tolSurface.sampling);
                renderToolbarWindow(title.c_str(), &m_profileService->m_showTolerancedProfileWindow, [&]() {
                    m_profileService->renderToolbar(profileUnits, false);
                    m_profileService->renderSurfaceProfilePlot("Toleranced", tolSurface, ImVec2(-1, -1));
                });
            }
        }

        if (m_profileService->m_showNominalProfileWindow) {
            if (nomSurface.isValid()) {
                ImGui::SetNextWindowSize(ImGuiUtils::DpiScaleVec2(kDetachedWindowSize), ImGuiCond_Once);
                std::string title = std::format("Nominal Surface Profile ({}°, {} pts)", nomSurface.angle, nomSurface.sampling);
                renderToolbarWindow(title.c_str(), &m_profileService->m_showNominalProfileWindow, [&]() {
                    m_profileService->renderToolbar(profileUnits, false);
                    m_profileService->renderSurfaceProfilePlot("Nominal", nomSurface, ImVec2(-1, -1));
                });
            }
        }

        if (m_profileService->m_showComparisonProfileWindow) {
            if (nomSurface.isValid() && tolSurface.isValid()) {
                ImGui::SetNextWindowSize(ImGuiUtils::DpiScaleVec2(kDetachedWindowSize), ImGuiCond_Once);
                renderToolbarWindow("Surface Profile Comparison", &m_profileService->m_showComparisonProfileWindow, [&]() {
                    m_profileService->renderToolbar(profileUnits, false);
                    m_profileService->renderProfileComparisonPlot("##DetachedProfiles", nomSurface, tolSurface, ImVec2(-1, -1));
                });
            }
        }

        if (m_profileService->m_showDeviationProfileWindow) {
            if (nomSurface.isValid() && tolSurface.isValid()) {
                ImGui::SetNextWindowSize(ImGuiUtils::DpiScaleVec2(kDetachedWindowSize), ImGuiCond_Once);
                renderToolbarWindow("Surface Profile Irregularity (PV)", &m_profileService->m_showDeviationProfileWindow, [&]() {
                    m_profileService->renderToolbar(profileUnits, false);
                    m_profileService->renderProfileDeviationPlot("##DetachedDeviation", nomSurface, tolSurface, ImVec2(-1, -1));
                });
            }
        }
    }

    {
        auto* mapService = m_irregularityMapService.get();

        if (mapService->m_showTolerancedSurfaceMap) {
            if (mapService->hasData()) {
                ImGui::SetNextWindowSize(ImGuiUtils::DpiScaleVec2(kDetachedWindowSize), ImGuiCond_Once);
                renderToolbarWindow("Surface Irregularity Map 3D", &mapService->m_showTolerancedSurfaceMap, [&]() {
                    m_profileService->renderToolbar(mapUnits, true,
                        &mapService->m_windowState.selectedColormapSurface,
                        &mapService->m_windowState.highlightWorstSurface,
                        mapService->m_windowState.worstColorSurface);
                    mapService->renderSurfacePlotLines(ImVec2(-1, -1));
                });
            } else {
                mapService->m_showTolerancedSurfaceMap = false;
            }
        }

        if (mapService->m_showDeviationSurfaceMap) {
            if (mapService->hasData() && mapService->m_nominalSurfaceData.isValid()) {
                ImGui::SetNextWindowSize(ImGuiUtils::DpiScaleVec2(kDetachedWindowSize), ImGuiCond_Once);
                renderToolbarWindow("Surface Irregularity Map 3D Deviation", &mapService->m_showDeviationSurfaceMap, [&]() {
                    m_profileService->renderToolbar(mapUnits, true,
                        &mapService->m_windowState.selectedColormapDeviation,
                        &mapService->m_windowState.highlightWorstDeviation,
                        mapService->m_windowState.worstColorDeviation);
                    mapService->renderDeviationSurfacePlotLines(ImVec2(-1, -1));
                });
            } else {
                mapService->m_showDeviationSurfaceMap = false;
            }
        }

        if (mapService->m_showWorstSectionProfile) {
            if (mapService->m_worstProfileData.isValid()) {
                ImGui::SetNextWindowSize(ImGuiUtils::DpiScaleVec2(kDetachedWindowSize), ImGuiCond_Once);
                renderToolbarWindow("Worst Section Profile", &mapService->m_showWorstSectionProfile, [&]() {
                    m_profileService->renderToolbar(profileUnits, false);
                    auto& wp = mapService->m_worstProfileData;
                    std::string title = std::format("Worst Section ({}°, {} pts)", wp.angle, wp.sampling);
                    m_profileService->renderSurfaceProfilePlot(title.c_str(), wp, ImVec2(-1, -1));
                });
            } else {
                mapService->m_showWorstSectionProfile = false;
            }
        }

        if (mapService->m_showWorstSectionDeviation) {
            if (mapService->m_worstProfileData.isValid()
                && mapService->m_nominalSurfaceData.isValid()) {
                ImGui::SetNextWindowSize(ImGuiUtils::DpiScaleVec2(kDetachedWindowSize), ImGuiCond_Once);
                renderToolbarWindow("Worst Section Deviation", &mapService->m_showWorstSectionDeviation, [&]() {
                    m_profileService->renderToolbar(profileUnits, false);
                    m_profileService->renderProfileDeviationPlot("##WorstDeviation",
                        mapService->m_nominalSurfaceData,
                        mapService->m_worstProfileData, ImVec2(-1, -1));
                });
            } else {
                mapService->m_showWorstSectionDeviation = false;
            }
        }
    }

    renderUpdatesPopup();
    renderAboutPopup();
    renderPreferencesDialog();
    renderConnectionLostPopup();

    m_uiOpMonitor.renderGlobalStatusBar();

    m_graphics.endFrame();
}

void GuiManager::updateDpiStyle(float dpiScale) {
    m_graphics.updateDpiStyle(dpiScale);
}

void GuiManager::renderLogs() {
    if (m_logsRenderer) {
        m_logsRenderer->render(m_logger, &m_graphics.getThemeManager());
    }
}

void GuiManager::renderAboutPopup() {
    if (m_aboutDialog) {
        m_aboutDialog->render();
    }
}

void GuiManager::renderUpdatesPopup() {
    if (m_updateChecker) {
        m_updateChecker->render();
    }
}

void GuiManager::renderPreferencesDialog() {
    if (m_preferencesDialog) {
        m_preferencesDialog->render();
    }
}

void GuiManager::renderConnectionLostPopup() {
    if (!m_ddeConnectionManager) return;

    if (m_ddeConnectionManager->hasConnectionLost() && !m_connectionLostDialog->isOpen()) {
        int lostIdx = m_ddeConnectionManager->getConnectionLostIndex();
        std::string reason = m_ddeConnectionManager->getConnectionLostReason();
        m_ddeConnectionManager->clearConnectionLost();
        m_connectionLostDialog->open(reason);
        m_pendingDisconnectIndex = lostIdx;
    }

    if (m_pendingDisconnectIndex >= 0 && !m_connectionLostDialog->isOpen()) {
        m_ddeConnectionManager->disconnect(m_pendingDisconnectIndex);
        m_pendingDisconnectIndex = -1;
    }

    if (m_connectionLostDialog) {
        m_connectionLostDialog->render();
    }
}

}

