#include "gui/gui.h"
#include <imgui.h>
#include "gui/sag_analysis_service.h"
#include "gui/menu_bar_controller.h"
#include "dde/dde_connection_manager.h"
#include "gui/sidebar_renderer.h"
#include "gui/content_router.h"
#include "gui/sag_analysis_controller.h"
#include "gui/app_info_dialog.h"
#include "gui/debug_log_viewer.h"

namespace gui {

    GuiManager::GuiManager(GLFWwindow* glfwWindow, HWND hwndClient, ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger)
: m_glfwWindow(glfwWindow)
, m_hwndClient(hwndClient)
, m_zemaxDDEClient(ddeClient)
, m_logger(logger)
, m_appInfoDialog(std::make_unique<AppInfoDialog>())
, m_debugLogViewer(std::make_unique<DebugLogViewer>())
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
    renderDockspace();
    if (m_sidebarRenderer) {
        m_sidebarRenderer->renderSidebar();
    }
    renderContent();
    renderDebugLog();
    renderDDEStatusBar();
    renderPopups();
    m_graphics.endFrame();
}

void GuiManager::renderDDEStatusBar() {
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    float barHeight = 64.0f;
    float barY = screenSize.y - barHeight;
    float textY = barY + (barHeight - 16.0f) / 2.0f;
    
    auto* drawList = ImGui::GetBackgroundDrawList();
    drawList->AddRectFilled(
        ImVec2(0.0f, barY),
        ImVec2(screenSize.x, screenSize.y),
        IM_COL32(40, 40, 40, 255),
        0.0f
    );
    drawList->AddLine(
        ImVec2(0.0f, barY),
        ImVec2(screenSize.x, barY),
        IM_COL32(100, 100, 100, 255),
        1.0f
    );
    
    bool connected = m_zemaxDDEClient && m_zemaxDDEClient->isConnected();
    ImU32 statusColor = connected ? IM_COL32(100, 255, 100, 255) : IM_COL32(255, 100, 100, 255);
    const char* statusPrefix = connected ? "● Connected" : "○ Disconnected";
    const char* statusSuffix = connected ? "| Server: ZEMAX" : "| Click 'Connect' to connect";
    
    drawList->AddText(ImVec2(8.0f, textY), statusColor, statusPrefix);
    float prefixWidth = ImGui::CalcTextSize(statusPrefix).x;
    drawList->AddText(ImVec2(8.0f + prefixWidth + 8.0f, textY), IM_COL32(180, 180, 180, 255), statusSuffix);
    
    if (connected) {
        ImVec2 btnMin = ImVec2(screenSize.x - 100.0f, barY + 20.0f);
        ImVec2 btnMax = ImVec2(screenSize.x - 20.0f, barY + barHeight - 1.0f);
        drawList->AddRectFilled(btnMin, btnMax, IM_COL32(180, 80, 80, 255), 4.0f);
        drawList->AddText(ImVec2(btnMin.x + 10.0f, textY), IM_COL32(255, 255, 255, 255), "Disconnect");
    } else {
        ImVec2 btnMin = ImVec2(screenSize.x - 80.0f, barY + 8.0f);
        ImVec2 btnMax = ImVec2(screenSize.x - 20.0f, barY + barHeight - 8.0f);
        drawList->AddRectFilled(btnMin, btnMax, IM_COL32(80, 180, 80, 255), 4.0f);
        drawList->AddText(ImVec2(btnMin.x + 10.0f, textY), IM_COL32(255, 255, 255, 255), "Connect");
    }
}

void GuiManager::renderDockspace() {
    const float menuHeight = ImGui::GetFrameHeight();
    const float statusBarHeight = 28.0f;
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    
    ImGui::SetNextWindowPos(ImVec2(0.0f, menuHeight));
    ImGui::SetNextWindowSize(ImVec2(
        displaySize.x,
        displaySize.y - menuHeight - statusBarHeight
    ));
    ImGui::Begin("MainDockSpace", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus
    );
    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();
}

void GuiManager::renderSidebar() {
    if (m_sidebarRenderer) {
        m_sidebarRenderer->renderSidebar();
    }
}

void GuiManager::renderContent() {
    if (m_contentRouter) {
        m_contentRouter->renderContent(m_currentPage, this);
    }
}

void GuiManager::renderDebugLog() {
    if (m_debugLogViewer) {
        m_debugLogViewer->render(m_logger);
    }
}

void GuiManager::renderPopups() {
    if (m_appInfoDialog) {
        m_appInfoDialog->render(m_showAboutPopup);
    }
}

void GuiManager::setPopupPosition() {
    if (m_appInfoDialog) {
        m_appInfoDialog->setPopupPosition();
    }
}

void GuiManager::updateDpiStyle(float dpiScale) {
    m_graphics.updateDpiStyle(dpiScale);
}

} // namespace gui
