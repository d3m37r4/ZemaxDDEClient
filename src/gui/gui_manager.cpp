#include "gui/gui.h"
#include "gui/constants.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cfloat>
#include <format>
#include <string>
#include "gui/surface_profile_service.h"
#include "gui/surface_irregularity_map_service.h"
#include "gui/menu_bar_controller.h"
#include "gui/imgui_utils.h"
#include "gui/dockable_windows_manager.h"
#include "gui/shortcuts.h"
#include "gui/notifications.h"
#include "dde/dde_connection_manager.h"
#include "dde/utils.h"
#include "windows_dockable/dde_status.h"
#include "logger/logger.h"

#ifdef ZEMAX_ENABLE_TEST_ENGINE
#include "test_engine/test_engine.h"
#endif

namespace gui {
namespace {
    // Welcome card texts: single source for the metrics helper and the
    // content renderer.
    constexpr const char* kWelcomeTitle = "Welcome to ZemaxDDEClient";
    constexpr const char* kWelcomeHint1 = "Start Zemax, then connect to the DDE server.";
    constexpr const char* kWelcomeHint2 = "Also in the menu: DDE > Connect to Zemax... (Ctrl+Shift+C).";
    constexpr const char* kWelcomeButton = ICON_FA_PLUG " Connect to Zemax...";

    // "[Slot N]" suffix for detached plot titles: which DDE connection slot
    // supplied the plotted data. Two different contributors sort and join
    // with "+"; unknown slots (-1) produce no tag.
    std::string slotTag(int a, int b = -1) {
        if (a < 0 && b < 0) return {};
        if (a < 0) return std::format(" [Slot {}]", b);
        if (b < 0 || b == a) return std::format(" [Slot {}]", a);
        return std::format(" [Slot {}+{}]", std::min(a, b), std::max(a, b));
    }
} // namespace

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

    // Cancel from the status bar: route by the task id itself. TaskSource
    // alone is ambiguous — a NominalSurfaceProfile task can be owned either
    // by the profile service or by the map service's nominal reference run.
    m_uiOpMonitor.setOnCancelTask([this](uint64_t taskId, app::models::TaskSource /*source*/) {
        if (m_profileService->cancelTask(taskId)) {
            return;
        }
        (void)m_irregularityMapService->cancelTask(taskId);
    });

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
    m_menuBarController->setShortcutsCallback([this]() {
        m_shortcutsHelpDialog->open();
    });
    m_menuBarController->setConnectCallback([this]() {
        openDdeConnect();
    });
    m_menuBarController->setDisconnectCallback([this]() {
        disconnectDdeSlot();
    });
    m_menuBarController->setSelectTargetCallback([this](int slot) {
        selectDdeTarget(slot);
    });
    m_menuBarController->setCycleTargetCallback([this]() {
        cycleDdeTarget();
    });
    m_ddeStatusRenderer = std::make_unique<DDEStatus>(m_ddeConnectionManager);
    m_logsRenderer = std::make_unique<Logs>();
    m_aboutDialog        = std::make_unique<AboutDialog>();
    m_shortcutsHelpDialog = std::make_unique<ShortcutsHelpDialog>();
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
    m_menuBarController->setThemeManager(&themeManager);
    m_ddeStatusRenderer->setThemeManager(&themeManager);
    m_ddeStatusRenderer->setLogger(&m_logger);
    m_updateChecker->setThemeManager(&themeManager);
    m_uiOpMonitor.setThemeManager(&themeManager);
    m_uiOpMonitor.setConnectionManager(m_ddeConnectionManager);
}

void GuiManager::render() {
    m_zemaxDDEClient = m_ddeConnectionManager ? m_ddeConnectionManager->getActiveClient() : nullptr;
    m_uiOpMonitor.setMonitor(m_zemaxDDEClient ? m_zemaxDDEClient->getOperationMonitor() : nullptr);

    if (m_ddeConnectionManager) {
        m_ddeConnectionManager->processAllTimeouts();
        m_ddeConnectionManager->checkAllConnectionHealth();
    }

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
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(mainViewport->Pos.x, mainViewport->Pos.y + navbarHeight));
    ImGui::SetNextWindowSize(ImVec2(
        mainViewport->Size.x,
        mainViewport->Size.y - navbarHeight - statusBarHeight
    ));
    // Under multi-viewport ImGui window coords are desktop coords, so an
    // anchoring SetNextWindowPos(0, ...) would spawn this host window as its
    // own platform window (seen as a second entry in the task bar). Pin it
    // to the main viewport so it can never own one.
    ImGui::SetNextWindowViewport(mainViewport->ID);
    // Square corners: the dockspace background drawn with the theme's
    // windowRounding shows rounded corners against the window edges even
    // when every dockable panel is hidden. The theme window padding is
    // applied explicitly so the host keeps it regardless of any style
    // stack pushed by outer callers.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImGui::GetStyle().WindowPadding);
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
    ImGui::PopStyleVar(2);

    if (m_pWndMgr) {
        m_pWndMgr->RenderAll();
    }

    auto& profileUnits = m_profileService->m_windowState.units;
    auto& mapUnits = m_irregularityMapService->m_windowState.units;

    {
        auto& tolSurface = m_profileService->m_tolerancedSurfaceData;
        auto& nomSurface = m_profileService->m_nominalSurfaceData;

        if (m_profileService->m_showTolerancedProfileWindow) {
            if (tolSurface.isValid()) {
                std::string title = std::format("Toleranced Surface Profile ({}°, {} pts){}",
                    tolSurface.angle, tolSurface.sampling, slotTag(tolSurface.sourceSlot));
                renderToolbarWindow(title.c_str(), &m_profileService->m_showTolerancedProfileWindow, "plot:ShowTolerancedProfileWindow", [&]() {
                    m_profileService->renderToolbar(profileUnits, false);
                    m_profileService->renderSurfaceProfilePlot("Toleranced", tolSurface, ImVec2(-1, -1));
                });
            }
        }

        if (m_profileService->m_showNominalProfileWindow) {
            if (nomSurface.isValid()) {
                std::string title = std::format("Nominal Surface Profile ({}°, {} pts){}",
                    nomSurface.angle, nomSurface.sampling, slotTag(nomSurface.sourceSlot));
                renderToolbarWindow(title.c_str(), &m_profileService->m_showNominalProfileWindow, "plot:ShowNominalProfileWindow", [&]() {
                    m_profileService->renderToolbar(profileUnits, false);
                    m_profileService->renderSurfaceProfilePlot("Nominal", nomSurface, ImVec2(-1, -1));
                });
            }
        }

        if (m_profileService->m_showComparisonProfileWindow) {
            if (nomSurface.isValid() && tolSurface.isValid()) {
                std::string title = std::format("Surface Profile Comparison{}",
                    slotTag(nomSurface.sourceSlot, tolSurface.sourceSlot));
                renderToolbarWindow(title.c_str(), &m_profileService->m_showComparisonProfileWindow, "plot:ShowComparisonProfileWindow", [&]() {
                    m_profileService->renderToolbar(profileUnits, false);
                    m_profileService->renderProfileComparisonPlot("##DetachedProfiles", nomSurface, tolSurface, ImVec2(-1, -1));
                });
            }
        }

        if (m_profileService->m_showDeviationProfileWindow) {
            if (nomSurface.isValid() && tolSurface.isValid()) {
                std::string title = std::format("Surface Profile Irregularity (PV){}",
                    slotTag(nomSurface.sourceSlot, tolSurface.sourceSlot));
                renderToolbarWindow(title.c_str(), &m_profileService->m_showDeviationProfileWindow, "plot:ShowDeviationProfileWindow", [&]() {
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
                std::string title = std::format("Surface Irregularity Map 3D{}",
                    slotTag(mapService->getProfiles().front().sourceSlot));
                renderToolbarWindow(title.c_str(), &mapService->m_showTolerancedSurfaceMap, "plot:ShowTolerancedSurfaceMap", [&]() {
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
                std::string title = std::format("Surface Irregularity Map 3D Deviation{}",
                    slotTag(mapService->m_nominalSurfaceData.sourceSlot,
                        mapService->getProfiles().front().sourceSlot));
                renderToolbarWindow(title.c_str(), &mapService->m_showDeviationSurfaceMap, "plot:ShowDeviationSurfaceMap", [&]() {
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
                std::string title = std::format("Worst Section Profile{}",
                    slotTag(mapService->m_worstProfileData.sourceSlot));
                renderToolbarWindow(title.c_str(), &mapService->m_showWorstSectionProfile, "plot:ShowWorstSectionProfile", [&]() {
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
                std::string title = std::format("Worst Section Deviation{}",
                    slotTag(mapService->m_nominalSurfaceData.sourceSlot,
                        mapService->m_worstProfileData.sourceSlot));
                renderToolbarWindow(title.c_str(), &mapService->m_showWorstSectionDeviation, "plot:ShowWorstSectionDeviation", [&]() {
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
    renderShortcutsHelpDialog();
    renderPreferencesDialog();
    renderConnectionLostPopup();

    m_uiOpMonitor.renderGlobalStatusBar();

    // DDE transitions: one toast per state change; the log stays the detailed
    // source of truth. The active slot's window title makes the toast concrete.
    static bool wasConnected = false;
    static bool wasConnectionLost = false;
    const bool connected = isDDEInitialized();
    if (connected != wasConnected) {
        wasConnected = connected;
        if (connected) {
            std::string title;
            if (m_ddeConnectionManager) {
                const int slot = m_ddeConnectionManager->getActiveIndex();
                if (const DDEConnection* conn = m_ddeConnectionManager->getConnection(slot);
                    conn && !conn->serverTitle.empty()) {
                    title = ZemaxDDE::wstring_to_utf8(conn->serverTitle);
                }
            }
            notify::success(title.empty() ? "Connected to Zemax"
                                          : std::format("Connected to \"{}\"", title));
        } else {
            notify::warning("Disconnected from Zemax");
        }
    }
    const bool connectionLost =
        m_ddeConnectionManager && m_ddeConnectionManager->hasConnectionLost();
    if (connectionLost && !wasConnectionLost) {
        notify::error(std::format("Connection lost: {}",
            m_ddeConnectionManager->getConnectionLostReason()));
    }
    wasConnectionLost = connectionLost;

    // Toasts inherit the active theme (window background, rounding, border) and
    // are drawn above every window by ImGuiNotify.
    ImGui::RenderNotifications();

#ifdef ZEMAX_ENABLE_TEST_ENGINE
    // The Test Engine UI must be rendered inside the ImGui frame, after the
    // application UI and before ImGui::Render().
    if (m_testEngine) {
        m_testEngine->renderUi();
    }
#endif

    m_graphics.endFrame(isAnyModalOpen());
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

void GuiManager::renderShortcutsHelpDialog() {
    if (m_shortcutsHelpDialog) {
        m_shortcutsHelpDialog->render();
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

ImVec2 GuiManager::welcomeWindowSize() const {
    WelcomeMetrics m;
    measureWelcome(m);
    const float blockW = std::max({m.icon.x, m.title.x, m.hint1.x, m.hint2.x, m.button.x});
    const float blockH = m.icon.y + m.gap + m.title.y + m.gap + m.hint1.y
                       + m.gap + m.hint2.y + m.gap + m.button.y;
    const float padX = ImGuiUtils::DpiScale(WELCOME_CARD_PADDING.x);
    const float padY = ImGuiUtils::DpiScale(WELCOME_CARD_PADDING.y);
    // Window chrome on top of the body: card padding, the title bar and the
    // 1px window border.
    return ImVec2(blockW + padX * 2.0f + 2.0f,
                  blockH + padY * 2.0f + ImGui::GetFrameHeight() + 2.0f);
}

void GuiManager::measureWelcome(WelcomeMetrics& m) const {
    ImFont* font = ImGui::GetFont();
    // The base font is already DPI-scaled, so the icon scale stays a plain ratio.
    const float baseFont = ImGui::GetFontSize();
    const float iconFont = baseFont * WELCOME_ICON_FONT_SCALE;

    m.wrap = ImGuiUtils::DpiScale(WELCOME_HINT_WRAP_WIDTH);
    m.gap = ImGuiUtils::DpiScale(WELCOME_ROW_GAP);
    m.icon = font->CalcTextSizeA(iconFont, FLT_MAX, 0.0f, ICON_FA_PLUG);
    m.title = font->CalcTextSizeA(baseFont, FLT_MAX, 0.0f, kWelcomeTitle);
    // Same measurement call chain as the TextUnformatted rendering below
    // (ImFontCalcTextSizeEx with a wrap width) so the rows center on what is
    // actually drawn.
    m.hint1 = ImGui::CalcTextSize(kWelcomeHint1, nullptr, false, m.wrap);
    m.hint2 = ImGui::CalcTextSize(kWelcomeHint2, nullptr, false, m.wrap);
    m.button = ImVec2(ImGui::CalcTextSize(kWelcomeButton).x + ImGui::GetStyle().FramePadding.x * 2.0f,
                      ImGui::GetFrameHeight());
}

void GuiManager::renderWelcomeContent() {
    const auto& sem = m_graphics.getThemeManager().semantic();
    ImFont* font = ImGui::GetFont();
    // The base font is already DPI-scaled, so the icon scale stays a plain ratio.
    const float baseFont = ImGui::GetFontSize();
    const float iconFont = baseFont * WELCOME_ICON_FONT_SCALE;

    // Center within the window's content area, not the dock region: the
    // window chrome (title bar, padding) belongs to DockableWindowsManager.
    const ImVec2 contentStart = ImGui::GetCursorStartPos();
    const ImVec2 winPos = ImGui::GetWindowPos();
    const ImVec2 areaMin(winPos.x + contentStart.x, winPos.y + contentStart.y);
    const ImVec2 areaSize = ImGui::GetContentRegionAvail();

    WelcomeMetrics m;
    measureWelcome(m);
    const float blockH = m.icon.y + m.gap + m.title.y + m.gap + m.hint1.y + m.gap + m.hint2.y + m.gap + m.button.y;
    const float centerX = areaMin.x + areaSize.x * 0.5f;
    float y = areaMin.y + (areaSize.y - blockH) * 0.5f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddText(font, iconFont, ImVec2(centerX - m.icon.x * 0.5f, y),
                      ImGui::GetColorU32(sem.info), ICON_FA_PLUG);
    y += m.icon.y + m.gap;

    ImGui::SetCursorScreenPos(ImVec2(centerX - m.title.x * 0.5f, y));
    ImGui::TextUnformatted(kWelcomeTitle);
    y += m.title.y + m.gap;

    // Hint rows are centered independently (each on the card axis) instead of
    // one ragged wrapped block: the "Also in the menu" sentence reads better on
    // its own line. Wrap safety for a narrow card: window-local wrap pos, see
    // CalcWrapWidthForPos (it shifts the value by window->Pos.x).
    ImGui::PushStyleColor(ImGuiCol_Text, sem.muted);
    auto renderHintRow = [&](const char* text, const ImVec2& size) {
        ImGui::SetCursorScreenPos(ImVec2(centerX - size.x * 0.5f, y));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + m.wrap);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        y += size.y + m.gap;
    };
    renderHintRow(kWelcomeHint1, m.hint1);
    renderHintRow(kWelcomeHint2, m.hint2);
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(centerX - m.button.x * 0.5f, y));
    ImGuiUtils::PushFilledButton(sem.successButton, sem.successButtonHover,
                                 sem.successButtonActive, sem.onAccent);
    if (ImGui::Button(kWelcomeButton, m.button)) {
        openDdeConnect();
    }
    ImGuiUtils::PopFilledButton();
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", shortcuts::tooltipFor("dde-connect").c_str());
    }
}

}

