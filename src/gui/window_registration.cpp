#include "window_registration.h"
#include "gui.h"
#include "window_manager.h"
#include "gui/constants.h"
#include "lib/imgui/imgui.h"

namespace {
    void RenderDDEStatusWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        bool isVisible = guiMgr->getWindowManager()->IsVisible(WindowID::DDEStatus);
        float dpiScale = ImGui::GetWindowDpiScale();
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(gui::DDE_STATUS_WINDOW_WIDTH_MIN * dpiScale,
                   gui::DDE_STATUS_WINDOW_HEIGHT_MIN * dpiScale),
            ImVec2(FLT_MAX, FLT_MAX)
        );
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
        float dpiScale = ImGui::GetWindowDpiScale();
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(gui::SYSTEM_INFO_WINDOW_WIDTH_MIN * dpiScale,
                   gui::SYSTEM_INFO_WINDOW_HEIGHT_MIN * dpiScale),
            ImVec2(FLT_MAX, FLT_MAX)
        );
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
        float dpiScale = ImGui::GetWindowDpiScale();
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(gui::SAG_ANALYSIS_WINDOW_WIDTH_MIN * dpiScale,
                   gui::SAG_ANALYSIS_WINDOW_HEIGHT_MIN * dpiScale),
            ImVec2(FLT_MAX, FLT_MAX)
        );
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
        float dpiScale = ImGui::GetWindowDpiScale();
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(gui::DEBUG_LOG_WINDOW_WIDTH_MIN * dpiScale,
                   gui::DEBUG_LOG_WINDOW_HEIGHT_MIN * dpiScale),
            ImVec2(FLT_MAX, FLT_MAX)
        );
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