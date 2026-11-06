#pragma once

#include <string>
#include <string_view>

#include "lib/imgui_notify/ImGuiNotify.hpp"

/// Thin wrappers over ImGuiNotify so call sites do not spell out toast types.
/// Rendering and theme styling live in GuiManager::renderNotifications().
namespace gui::notify {

    namespace detail {
        inline void push(ImGuiToastType type, std::string_view text, int durationMs) {
            const std::string message(text);
            ImGui::InsertNotification({type, durationMs, "%s", message.c_str()});
        }
    }

    inline void info(std::string_view text, int durationMs = 2500) {
        detail::push(ImGuiToastType::Info, text, durationMs);
    }

    inline void success(std::string_view text, int durationMs = 2500) {
        detail::push(ImGuiToastType::Success, text, durationMs);
    }

    inline void warning(std::string_view text, int durationMs = 3500) {
        detail::push(ImGuiToastType::Warning, text, durationMs);
    }

    inline void error(std::string_view text, int durationMs = 4500) {
        detail::push(ImGuiToastType::Error, text, durationMs);
    }

    /// Short confirmation for clipboard actions: "Copied: <what>".
    inline void copied(std::string_view what, int durationMs = 1500) {
        info("Copied: " + std::string(what), durationMs);
    }

}
