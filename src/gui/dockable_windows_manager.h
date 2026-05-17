#pragma once

#include <functional>
#include <string>
#include <vector>

enum class WindowID {
    SagAnalysis,
    DebugLog,
    SystemInfo,
    DDEStatus,
};

enum class WindowCategory {
    Tools,
    Info,
    DDE,
};

namespace gui {
    class GuiManager;
}

struct DockableWindowConfig {
    WindowID id;
    WindowCategory category;
    const char* name;
    bool isVisible;
    int order;
};

inline constexpr DockableWindowConfig DockableWindows[] = {
    { WindowID::DDEStatus, WindowCategory::DDE, "DDE Status", true, 0 },
    { WindowID::SystemInfo, WindowCategory::Tools, "Optical System Information", true, 0 },
    { WindowID::SagAnalysis, WindowCategory::Tools, "Surface Sag Cross Section Analysis", false, 1 },
    { WindowID::DebugLog, WindowCategory::Info, "Debug Log", true, 0 },
};

class DockableWindowsManager {
    public:
        DockableWindowsManager();
        ~DockableWindowsManager();

        void RegisterWindow(WindowID id, const char* name, WindowCategory category, std::function<void()> renderFn, int order = 0, bool isVisible = true);

        bool IsVisible(WindowID id) const;
        void SetVisible(WindowID id, bool visible);

        void RenderAll();

        void LoadState();
        void SaveState();

        const std::vector<std::pair<WindowID, std::string>> GetNames() const;
        const std::vector<std::pair<WindowID, bool>> GetVisibilities() const;
        std::vector<WindowID> GetIDsByCategory(WindowCategory category) const;

        void RegisterDockableWindows(gui::GuiManager* guiMgr);

    private:
        struct WindowEntry {
            WindowID id;
            WindowCategory category;
            std::function<void()> render;
            std::string name;
            bool isVisible;
            int order;
        };

        std::vector<WindowEntry> windows_;
};