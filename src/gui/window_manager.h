#pragma once

#include <unordered_map>
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

enum class WindowType {
    Dockable,
    Popup,
};

class WindowManager {
    public:
        WindowManager();
        ~WindowManager();

        void RegisterWindow(WindowID id, const char* name, WindowCategory category, WindowType type, std::function<void()> renderFn);

        bool IsVisible(WindowID id) const;
        void SetVisible(WindowID id, bool visible);

        void RenderAll();

        void LoadState();
        void SaveState();

        const std::unordered_map<WindowID, std::string>& GetNames() const { return names_; }
        const std::unordered_map<WindowID, bool>& GetVisibilities() const { return visibility_; }
        std::vector<WindowID> GetIDsByCategory(WindowCategory category) const;
        bool IsPopup(WindowID id) const;
        void SetWindowOrder(WindowID id, int order);

    private:
        struct WindowInfo { WindowCategory category; WindowType type; std::function<void()> render; };
        std::unordered_map<WindowID, WindowInfo> windows_;
        std::unordered_map<WindowID, std::string> names_;
        std::unordered_map<WindowID, bool> visibility_;
        std::unordered_map<WindowID, int> displayOrder_;
};