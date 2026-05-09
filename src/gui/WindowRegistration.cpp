#include "WindowRegistration.h"
#include "gui.h"
#include "lib/imgui/imgui.h"

namespace {
    void RenderSystemInfoWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("System Info", nullptr)) {
            guiMgr->renderPageOpticalSystemInfo();
        }
        ImGui::End();
    }

    void RenderSagAnalysisWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Sag Analysis", nullptr)) {
            guiMgr->renderPageSurfaceSagAnalysis();
        }
        ImGui::End();
    }

    void RenderDebugLogWindow(gui::GuiManager* guiMgr) {
        if (!guiMgr) return;
        guiMgr->renderDebugLog();
    }
}

void RegisterAllWindows(WindowManager& mgr, gui::GuiManager* guiMgr) {
    mgr.RegisterWindow(WindowID::SagAnalysis, "Sag Analysis", [guiMgr]() {
        RenderSagAnalysisWindow(guiMgr);
    });
    mgr.SetVisible(WindowID::SagAnalysis, false); // closed by default

    mgr.RegisterWindow(WindowID::DebugLog, "Debug Log", [guiMgr]() {
        RenderDebugLogWindow(guiMgr);
    });

    mgr.RegisterWindow(WindowID::SystemInfo, "System Info", [guiMgr]() {
        RenderSystemInfoWindow(guiMgr);
    });
}