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

class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    void RegisterWindow(WindowID id, const char* name, WindowCategory category, std::function<void()> renderFn);

    bool IsVisible(WindowID id) const;
    void SetVisible(WindowID id, bool visible);

    void RenderAll();

    void LoadState();
    void SaveState();

    const std::unordered_map<WindowID, std::string>& GetNames() const { return names_; }
    const std::unordered_map<WindowID, bool>& GetVisibilities() const { return visibility_; }
    std::vector<WindowID> GetIDsByCategory(WindowCategory category) const;

private:
    struct WindowInfo { WindowCategory category; std::function<void()> render; };
    std::unordered_map<WindowID, WindowInfo> windows_;
    std::unordered_map<WindowID, std::string> names_;
    std::unordered_map<WindowID, bool> visibility_;
};