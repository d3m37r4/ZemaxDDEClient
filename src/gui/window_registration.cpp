#include "window_registration.h"
#include "gui.h"
#include "window_manager.h"
#include "gui/constants.h"
#include "lib/imgui/imgui.h"

namespace {
    void SetDpiScaledWindowConstraints(float minWidth, float minHeight) {
        float dpiScale = ImGui::GetWindowDpiScale();
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(minWidth * dpiScale, minHeight * dpiScale),
            ImVec2(FLT_MAX, FLT_MAX)
        );
    }

    void RenderDDEStatusWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        bool isVisible = guiMgr->getWindowManager()->IsVisible(WindowID::DDEStatus);
        SetDpiScaledWindowConstraints(gui::DDE_STATUS_WINDOW_WIDTH_MIN, gui::DDE_STATUS_WINDOW_HEIGHT_MIN);
        if (ImGui::Begin("DDE Status", &isVisible,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            if (guiMgr->getDDEStatusRenderer()) {
                guiMgr->getDDEStatusRenderer()->render(guiMgr->getLogger());
            }
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::DDEStatus, false);
        }
    }

    void RenderSystemInfoWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        bool isVisible = guiMgr->getWindowManager()->IsVisible(WindowID::SystemInfo);
        SetDpiScaledWindowConstraints(gui::SYSTEM_INFO_WINDOW_WIDTH_MIN, gui::SYSTEM_INFO_WINDOW_HEIGHT_MIN);
        if (ImGui::Begin("Optical System Information", &isVisible)) {
            guiMgr->renderOpticalSystemInfo();
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::SystemInfo, false);
        }
    }

    void RenderSagAnalysisWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        bool isVisible = guiMgr->getWindowManager()->IsVisible(WindowID::SagAnalysis);
        SetDpiScaledWindowConstraints(gui::SAG_ANALYSIS_WINDOW_WIDTH_MIN, gui::SAG_ANALYSIS_WINDOW_HEIGHT_MIN);
        if (ImGui::Begin("Surface Sag Cross Section Analysis", &isVisible)) {
            guiMgr->renderSurfaceSagAnalysis();
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::SagAnalysis, false);
        }
    }

    void RenderDebugLogWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        bool isVisible = guiMgr->getWindowManager()->IsVisible(WindowID::DebugLog);
        SetDpiScaledWindowConstraints(gui::DEBUG_LOG_WINDOW_WIDTH_MIN, gui::DEBUG_LOG_WINDOW_HEIGHT_MIN);
        if (ImGui::Begin("Debug Log", &isVisible)) {
            guiMgr->renderDebugLog();
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::DebugLog, false);
        }
    }
}

void RegisterAllWindows(WindowManager& mgr, gui::GuiManager* guiMgr) {
    mgr.RegisterWindow(WindowID::DDEStatus, "DDE Status", WindowCategory::DDE, [guiMgr]() {
        RenderDDEStatusWindow(guiMgr);
    });

    mgr.RegisterWindow(WindowID::SystemInfo, "Optical System Information", WindowCategory::Tools, [guiMgr]() {
        RenderSystemInfoWindow(guiMgr);
    });

    mgr.RegisterWindow(WindowID::SagAnalysis, "Surface Sag Cross Section Analysis", WindowCategory::Tools, [guiMgr]() {
        RenderSagAnalysisWindow(guiMgr);
    });
    mgr.SetVisible(WindowID::SagAnalysis, false);

    mgr.RegisterWindow(WindowID::DebugLog, "Debug Log", WindowCategory::Info, [guiMgr]() {
        RenderDebugLogWindow(guiMgr);
    });
}