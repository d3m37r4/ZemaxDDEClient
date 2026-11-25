#include "gui/viewport_placement.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <unordered_map>

namespace gui::placement {
    namespace {
        /// Rect a maximized panel came from, keyed by platform viewport ID.
        std::unordered_map<ImGuiID, Rect> g_savedRects;
        /// Rect waiting for its panel chrome, keyed by platform viewport ID.
        std::unordered_map<ImGuiID, Rect> g_pending;

        Rect fromRect(const RECT& rect) {
            return {rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top};
        }

        HWND nativeWindow(const ImGuiViewport* viewport) {
            return viewport != nullptr && viewport->PlatformHandleRaw != nullptr
                ? static_cast<HWND>(viewport->PlatformHandleRaw) : nullptr;
        }
    } // namespace

    Rect workAreaAt(int x, int y) {
        MONITORINFO info{};
        info.cbSize = sizeof(info);
        const POINT point{x, y};
        if (!GetMonitorInfoW(MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST), &info)) {
            return {};
        }
        return fromRect(info.rcWork);
    }

    Rect workAreaOf(const ImGuiViewport* viewport) {
        const HWND hwnd = nativeWindow(viewport);
        if (hwnd == nullptr) {
            return {};
        }
        MONITORINFO info{};
        info.cbSize = sizeof(info);
        if (!GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &info)) {
            return {};
        }
        return fromRect(info.rcWork);
    }

    void requestPlacement(ImGuiViewport* viewport, const Rect& rect) {
        if (viewport == nullptr || rect.w <= 0 || rect.h <= 0) {
            return;
        }
        g_pending[viewport->ID] = rect;
    }

    bool takePlacement(ImGuiViewport* viewport, Rect& out) {
        if (viewport == nullptr) {
            return false;
        }
        const auto it = g_pending.find(viewport->ID);
        if (it == g_pending.end()) {
            return false;
        }
        out = it->second;
        g_pending.erase(it);
        return true;
    }

    void toggleMaximize(ImGuiViewport* viewport, const Rect& current) {
        if (viewport == nullptr) {
            return;
        }
        const auto saved = g_savedRects.find(viewport->ID);
        if (saved != g_savedRects.end()) {
            const Rect restore = saved->second;
            g_savedRects.erase(saved);
            requestPlacement(viewport, restore);
            return;
        }
        const Rect work = workAreaOf(viewport);
        if (work.w <= 0 || work.h <= 0 || current.w <= 0 || current.h <= 0) {
            return;
        }
        g_savedRects.emplace(viewport->ID, current);
        requestPlacement(viewport, work);
    }

    bool isMaximized(const ImGuiViewport* viewport) {
        return viewport != nullptr && g_savedRects.contains(viewport->ID);
    }

    void pruneSavedRects() {
        if (g_savedRects.empty() && g_pending.empty()) {
            return;
        }
        for (auto it = g_savedRects.begin(); it != g_savedRects.end();) {
            it = ImGui::FindViewportByID(it->first) == nullptr
                ? g_savedRects.erase(it) : std::next(it);
        }
        for (auto it = g_pending.begin(); it != g_pending.end();) {
            it = ImGui::FindViewportByID(it->first) == nullptr
                ? g_pending.erase(it) : std::next(it);
        }
    }
} // namespace gui::placement
