#pragma once

#include <unordered_map>
#include <functional>
#include <string>

enum class WindowID {
    SagAnalysis,
    DebugLog,
    SystemInfo,
};

class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    void RegisterWindow(WindowID id, const char* name, std::function<void()> renderFn);

    bool IsVisible(WindowID id) const;
    void SetVisible(WindowID id, bool visible);

    void RenderAll();

    void LoadState();
    void SaveState();

    const std::unordered_map<WindowID, std::string>& GetNames() const { return names_; }
    const std::unordered_map<WindowID, bool>& GetVisibilities() const { return visibility_; }

private:
    struct WindowInfo { std::function<void()> render; };
    std::unordered_map<WindowID, WindowInfo> windows_;
    std::unordered_map<WindowID, std::string> names_;
    std::unordered_map<WindowID, bool> visibility_;
};