#include "gui/log_filter_util.h"

#include <cctype>
#include <string>

namespace gui::logs {
    namespace {
        char toLower(char c) {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        bool containsNoCaseImpl(std::string_view haystack, std::string_view needle) {
            if (needle.empty()) {
                return true;
            }
            if (needle.size() > haystack.size()) {
                return false;
            }
            for (size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
                size_t j = 0;
                while (j < needle.size() && toLower(haystack[i + j]) == toLower(needle[j])) {
                    ++j;
                }
                if (j == needle.size()) {
                    return true;
                }
            }
            return false;
        }
    }

    LogLevel levelOf(std::string_view line) {
        const size_t timestampEnd = line.find(']');
        if (timestampEnd == std::string_view::npos) {
            return LogLevel::Unknown;
        }
        size_t pos = timestampEnd + 1;
        while (pos < line.size() && line[pos] == ' ') {
            ++pos;
        }
        if (pos >= line.size() || line[pos] != '[') {
            return LogLevel::Unknown;
        }
        const size_t tagEnd = line.find(']', pos + 1);
        if (tagEnd == std::string_view::npos || tagEnd == pos + 1) {
            return LogLevel::Unknown;
        }
        const std::string_view tag = line.substr(pos + 1, tagEnd - pos - 1);
        if (tag == "APP") {
            return LogLevel::App;
        }
        if (tag == "DDE") {
            return LogLevel::Dde;
        }
        if (tag == "GUI") {
            return LogLevel::Gui;
        }
        return LogLevel::Service;
    }

    bool containsNoCase(std::string_view haystack, std::string_view needle) {
        return containsNoCaseImpl(haystack, needle);
    }

    bool looksLikeProblem(std::string_view line) {
        static constexpr std::string_view kWords[] = {
            "error", "failed", "timeout", "cancel",
        };
        for (const std::string_view word : kWords) {
            if (containsNoCaseImpl(line, word)) {
                return true;
            }
        }
        return false;
    }

}
