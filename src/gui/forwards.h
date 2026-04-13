#pragma once

namespace gui {
    enum class GuiPage {
        OpticalSystemInfo,      // = 0
        SurfaceSagAnalysis      // = 1
    };

    struct GuiPageInfo {
        GuiPage id;
        const char* title;
    };

    constexpr GuiPageInfo GUI_PAGES[] = {
        {GuiPage::OpticalSystemInfo, "Optical System Information"},
        {GuiPage::SurfaceSagAnalysis, "Surface Sag Cross Section Analysis"}
    };

    constexpr size_t GUI_PAGES_COUNT = sizeof(GUI_PAGES) / sizeof(GUI_PAGES[0]);

    class GuiManager;
}
