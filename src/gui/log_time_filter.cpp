#include "gui/log_time_filter.h"

namespace gui::logs {
    namespace {
        bool readDigits(std::string_view text, size_t offset, size_t count, int& out) {
            if (offset + count > text.size()) {
                return false;
            }
            int value = 0;
            for (size_t i = 0; i < count; ++i) {
                const char c = text[offset + i];
                if (c < '0' || c > '9') {
                    return false;
                }
                value = value * 10 + (c - '0');
            }
            out = value;
            return true;
        }
    }

    std::optional<std::chrono::system_clock::time_point> parseLogTimestamp(std::string_view entry) {
        // Layout produced by Logger: "[04.11.2026 - 21:32:18] ..." (23 chars).
        if (entry.size() < 23 || entry[0] != '[') {
            return std::nullopt;
        }
        int day = 0, month = 0, year = 0, hour = 0, minute = 0, second = 0;
        if (entry[3] != '.' || entry[6] != '.' || entry[11] != ' ' || entry[12] != '-' ||
            entry[13] != ' ' || entry[16] != ':' || entry[19] != ':' || entry[22] != ']') {
            return std::nullopt;
        }
        if (!readDigits(entry, 1, 2, day) || !readDigits(entry, 4, 2, month) ||
            !readDigits(entry, 7, 4, year) || !readDigits(entry, 14, 2, hour) ||
            !readDigits(entry, 17, 2, minute) || !readDigits(entry, 20, 2, second)) {
            return std::nullopt;
        }
        if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31 ||
            hour > 23 || minute > 59 || second > 59) {
            return std::nullopt;
        }
        using namespace std::chrono;
        const year_month_day date{std::chrono::year{year}, std::chrono::month{static_cast<unsigned>(month)},
                                  std::chrono::day{static_cast<unsigned>(day)}};
        if (!date.ok()) {
            return std::nullopt;
        }
        return sys_days{date} + hours{hour} + minutes{minute} + seconds{second};
    }

    bool inTimeRange(std::chrono::system_clock::time_point timestamp,
                     const std::optional<std::chrono::system_clock::time_point>& from,
                     const std::optional<std::chrono::system_clock::time_point>& to) {
        if (from && timestamp < *from) {
            return false;
        }
        if (to && timestamp > *to) {
            return false;
        }
        return true;
    }

}
