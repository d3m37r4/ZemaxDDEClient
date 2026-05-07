#include "WindowRegistration.h"
#include "gui.h"

void RegisterAllWindows(WindowManager& mgr, gui::GuiManager* guiMgr) {
    mgr.RegisterWindow(WindowID::SagAnalysis, "Sag Analysis", [guiMgr]() {
        if (guiMgr) guiMgr->renderPageSurfaceSagAnalysis();
    });

    mgr.RegisterWindow(WindowID::DebugLog, "Debug Log", [guiMgr]() {
        if (guiMgr) guiMgr->renderDebugLog();
    });

    mgr.RegisterWindow(WindowID::SystemInfo, "System Info", [guiMgr]() {
        if (guiMgr) guiMgr->renderPageOpticalSystemInfo();
    });
}