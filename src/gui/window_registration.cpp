#include "window_registration.h"
#include "gui.h"
#include "window_manager.h"
#include "gui/constants.h"
#include "lib/imgui/imgui.h"

namespace {
    void RenderSystemInfoWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        bool isVisible = guiMgr->getWindowManager()->IsVisible(WindowID::SystemInfo);
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("System Info", &isVisible)) {
            guiMgr->renderPageOpticalSystemInfo();
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::SystemInfo, false);
        }
    }

    void RenderSagAnalysisWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        bool isVisible = guiMgr->getWindowManager()->IsVisible(WindowID::SagAnalysis);
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Sag Analysis", &isVisible)) {
            guiMgr->renderPageSurfaceSagAnalysis();
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::SagAnalysis, false);
        }
    }

    void RenderDebugLogWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        bool isVisible = guiMgr->getWindowManager()->IsVisible(WindowID::DebugLog);
        guiMgr->renderDebugLog(&isVisible);
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::DebugLog, false);
        }
    }

    void RenderDdeStatusWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        bool isVisible = guiMgr->getWindowManager()->IsVisible(WindowID::DdeStatus);
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(gui::DDE_STATUS_WINDOW_WIDTH_MIN, gui::DDE_STATUS_WINDOW_HEIGHT_MIN),
            ImVec2(FLT_MAX, FLT_MAX)
        );
        if (ImGui::Begin("DDE Status", &isVisible,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            if (guiMgr->getDdeStatusRenderer()) {
                guiMgr->getDdeStatusRenderer()->renderDdeStatus(guiMgr->getDdeClient(), guiMgr->getLogger());
            }
        }
        ImGui::End();
        if (!isVisible) {
            guiMgr->getWindowManager()->SetVisible(WindowID::DdeStatus, false);
        }
    }
}

void RegisterAllWindows(WindowManager& mgr, gui::GuiManager* guiMgr) {
    mgr.RegisterWindow(WindowID::SagAnalysis, "Sag Analysis", WindowCategory::Tools, [guiMgr]() {
        RenderSagAnalysisWindow(guiMgr);
    });
    mgr.SetVisible(WindowID::SagAnalysis, false);

    mgr.RegisterWindow(WindowID::DebugLog, "Debug Log", WindowCategory::Tools, [guiMgr]() {
        RenderDebugLogWindow(guiMgr);
    });

    mgr.RegisterWindow(WindowID::SystemInfo, "System Info", WindowCategory::Tools, [guiMgr]() {
        RenderSystemInfoWindow(guiMgr);
    });

    mgr.RegisterWindow(WindowID::DdeStatus, "DDE Status", WindowCategory::DDE, [guiMgr]() {
        RenderDdeStatusWindow(guiMgr);
    });
}