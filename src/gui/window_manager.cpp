#include "window_manager.h"
#include "app/config_path.h"
#include <fstream>
#include <algorithm>
#include <limits>
#include <nlohmann/json.hpp>

WindowManager::WindowManager() = default;
WindowManager::~WindowManager() = default;

void WindowManager::RegisterWindow(WindowID id, const char* name, WindowCategory category, WindowType type, std::function<void()> renderFn) {
    windows_[id] = {category, type, std::move(renderFn)};
    names_[id] = name;
    if (visibility_.find(id) == visibility_.end())
        visibility_[id] = type == WindowType::Dockable;
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
    std::ifstream in(app::getWindowStatePath());
    if (!in) return;
    nlohmann::json j; in >> j;
    for (auto &el : j.items()) {
        const std::string& name = el.key();
        bool visible = el.value().get<bool>();
        // Find WindowID by name
        for (auto &pair : names_) {
            if (pair.second == name) {
                visibility_[pair.first] = visible;
                break;
            }
        }
    }
}

void WindowManager::SaveState() {
    nlohmann::json j;
    for (auto &pair : visibility_) {
        const std::string& name = names_.at(pair.first);
        j[name] = pair.second;
    }
    std::ofstream out(app::getWindowStatePath());
    out << j.dump(4);
}

std::vector<WindowID> WindowManager::GetIDsByCategory(WindowCategory category) const {
    std::vector<WindowID> result;
    for (auto &pair : windows_) {
        if (pair.second.category == category && pair.second.type == WindowType::Dockable) {
            result.push_back(pair.first);
        }
    }
    std::stable_sort(result.begin(), result.end(), [this](WindowID a, WindowID b) {
        int orderA = displayOrder_.count(a) ? displayOrder_.at(a) : std::numeric_limits<int>::max();
        int orderB = displayOrder_.count(b) ? displayOrder_.at(b) : std::numeric_limits<int>::max();
        return orderA < orderB;
    });
    return result;
}

void WindowManager::SetWindowOrder(WindowID id, int order) {
    displayOrder_[id] = order;
}

bool WindowManager::IsPopup(WindowID id) const {
    auto it = windows_.find(id);
    if (it == windows_.end()) return false;
    return it->second.type == WindowType::Popup;
}