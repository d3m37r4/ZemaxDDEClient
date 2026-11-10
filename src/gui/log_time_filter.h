#pragma once

#include <chrono>
#include <optional>
#include <string_view>

/// Timestamp helpers for the Logs panel filter. Kept free of ImGui so the
/// parsing and range rules are unit-testable.
namespace gui::logs {

    enum class TimePreset { All, Last5Min, Last15Min, LastHour, Last24Hours, Custom };

    /// Parses the "[dd.mm.yyyy - HH:MM:SS]" prefix written by Logger.
    /// Returns nullopt for anything that does not match the format exactly
    /// (such lines are treated as "time unknown" by the filter).
    std::optional<std::chrono::system_clock::time_point> parseLogTimestamp(std::string_view entry);

    /// Inclusive range check; an empty bound means "unbounded".
    bool inTimeRange(std::chrono::system_clock::time_point timestamp,
                     const std::optional<std::chrono::system_clock::time_point>& from,
                     const std::optional<std::chrono::system_clock::time_point>& to);

}
