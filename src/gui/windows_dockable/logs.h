#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "gui/log_time_filter.h"
#include "gui/log_filter_util.h"

class Logger;
class ThemeManager;

namespace gui {
    class Logs {
    public:
        void render(Logger& logger, const ThemeManager* themeManager);

    private:
        // Single panel - the toggle state lives in one instance.
        bool m_autoscroll = true;

        // One-shot "jump to the very bottom" request (enabling the toggle);
        // executed in the content child, since SetScrollY would target the header.
        bool m_jumpToBottom = false;

        // User requested a log clear - the confirmation modal opens on the
        // nearest frame; the flag is cleared when the modal closes in any way
        // (Cancel / Clear / close button / Esc).
        bool m_confirmDelete = false;

        // Time filter. Presets are relative to "now"; Custom uses the two text
        // fields, parsed on Apply (invalid input keeps the popup open).
        logs::TimePreset m_preset = logs::TimePreset::All;
        std::array<char, 24> m_fromText{};
        std::array<char, 24> m_toText{};
        std::optional<std::chrono::system_clock::time_point> m_customFrom;
        std::optional<std::chrono::system_clock::time_point> m_customTo;
        std::string m_filterError;

        // Free-text search (case-insensitive, whole line). Shown in its own
        // popup; the text survives closing it so the filter stays visible.
        std::array<char, 128> m_searchText{};

        // Level groups of the filter popup.
        bool m_showApp = true;
        bool m_showDde = true;
        bool m_showGui = true;
        bool m_showService = true;
        bool m_showOther = true;

        // Parsed metadata cache; rebuilt only when the log size changes so the
        // per-frame filtering stays a set of cheap checks.
        struct CacheEntry {
            std::optional<std::chrono::system_clock::time_point> time;
            logs::LogLevel level = logs::LogLevel::Unknown;
        };
        std::vector<CacheEntry> m_cache;
        size_t m_cacheSize = static_cast<size_t>(-1);

        // Multi-selection over visible entries, stored by original index.
        std::set<size_t> m_selected;
        size_t m_selectionAnchor = 0;
        bool m_hasSelectionAnchor = false;
    };
}
