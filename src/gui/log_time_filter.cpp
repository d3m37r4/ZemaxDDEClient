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
        // Accepts both shapes:
        //   "[04.11.2026 - 21:32:18] ..."  (a Logger line)
        //   "04.11.2026 - 21:32:18"        (typed into the filter)
        // Surrounding spaces are ignored; anything else must match exactly.
        size_t begin = 0;
        while (begin < entry.size() && entry[begin] == ' ') {
            ++begin;
        }
        size_t end = entry.size();
        while (end > begin && entry[end - 1] == ' ') {
            --end;
        }
        std::string_view text = entry.substr(begin, end - begin);

        const bool bracketed = !text.empty() && text[0] == '[';
        if (bracketed) {
            text.remove_prefix(1);
        }
        if (text.size() < 21) {
            return std::nullopt;
        }
        if (bracketed) {
            if (text.size() < 22 || text[21] != ']') {
                return std::nullopt;
            }
        } else if (text.size() != 21) {
            return std::nullopt;
        }
        if (text[2] != '.' || text[5] != '.' || text[10] != ' ' || text[11] != '-' ||
            text[12] != ' ' || text[15] != ':' || text[18] != ':') {
            return std::nullopt;
        }
        int day = 0, month = 0, year = 0, hour = 0, minute = 0, second = 0;
        if (!readDigits(text, 0, 2, day) || !readDigits(text, 3, 2, month) ||
            !readDigits(text, 6, 4, year) || !readDigits(text, 13, 2, hour) ||
            !readDigits(text, 16, 2, minute) || !readDigits(text, 19, 2, second)) {
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
