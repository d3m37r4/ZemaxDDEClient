#pragma once

#include <string_view>

/// Pure helpers for the Logs panel filters (search text, level detection).
/// Kept free of ImGui so the rules are unit-testable.
namespace gui::logs {

    enum class LogLevel { App, Dde, Gui, Service, Unknown };

    /// Level tag that follows the timestamp: "[...] [DDE] ...".
    /// APP/DDE/GUI map to their groups, any other tag is a service module,
    /// a line without a tag is Unknown.
    LogLevel levelOf(std::string_view line);

    /// Case-insensitive substring test; an empty needle matches everything.
    bool containsNoCase(std::string_view haystack, std::string_view needle);

    /// True for lines worth highlighting: error/failed/timeout/cancel words.
    bool looksLikeProblem(std::string_view line);

    /// True for lines that report a completed action (completed, connected,
    /// saved, ready, ...). Check the problem words first: a failed line may
    /// still contain a success word ("failed to load").
    bool looksLikeSuccess(std::string_view line);

}
