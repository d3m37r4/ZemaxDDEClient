#include "WindowManager.h"
#include <fstream>
#include <nlohmann/json.hpp>

WindowManager::WindowManager() = default;
WindowManager::~WindowManager() = default;

void WindowManager::RegisterWindow(WindowID id, const char* name, std::function<void()> renderFn) {
    windows_[id].render = std::move(renderFn);
    names_[id] = name;
    if (visibility_.find(id) == visibility_.end())
        visibility_[id] = true;
}

bool WindowManager::IsVisible(WindowID id) const { return visibility_.at(id); }
void WindowManager::SetVisible(WindowID id, bool visible) { visibility_[id] = visible; }

void WindowManager::RenderAll() {
    for (auto &pair : windows_) {
        if (visibility_[pair.first] && pair.second.render)
            pair.second.render();
    }
}

void WindowManager::LoadState() {
    std::ifstream in("window_state.json");
    if (!in) return;
    nlohmann::json j; in >> j;
    for (auto &el : j.items()) {
        WindowID id = static_cast<WindowID>(std::stoi(el.key()));
        visibility_[id] = el.value().get<bool>();
    }
}

void WindowManager::SaveState() {
    nlohmann::json j;
    for (auto &pair : visibility_)
        j[std::to_string(static_cast<int>(pair.first))] = pair.second;
    std::ofstream out("window_state.json");
    out << j.dump(4);
}