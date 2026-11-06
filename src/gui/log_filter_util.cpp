#include "gui/log_filter_util.h"

#include <cctype>
#include <iterator>
#include <string>

namespace gui::logs {
    namespace {
        char toLower(char c) {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        bool isWordChar(char c) {
            return std::isalnum(static_cast<unsigned char>(c)) != 0;
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

        /// Matches a word only as a standalone token: no alphanumeric neighbours
        /// and not a key=value parameter ("timeout=2000ms" is a parameter, not
        /// a failure).
        bool containsWordNoCase(std::string_view haystack, std::string_view word) {
            if (word.empty() || word.size() > haystack.size()) {
                return false;
            }
            for (size_t i = 0; i + word.size() <= haystack.size(); ++i) {
                bool match = true;
                for (size_t j = 0; j < word.size(); ++j) {
                    if (toLower(haystack[i + j]) != toLower(word[j])) {
                        match = false;
                        break;
                    }
                }
                if (!match) {
                    continue;
                }
                const bool beforeOk = i == 0 || !isWordChar(haystack[i - 1]);
                const size_t end = i + word.size();
                const bool afterOk = end == haystack.size() ||
                                     (!isWordChar(haystack[end]) && haystack[end] != '=');
                if (beforeOk && afterOk) {
                    return true;
                }
            }
            return false;
        }

        bool containsAnyWord(std::string_view line, const std::string_view* words, size_t count) {
            for (size_t i = 0; i < count; ++i) {
                if (containsWordNoCase(line, words[i])) {
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
            "error", "failed", "failure", "timeout", "exception", "cancelled", "canceled",
        };
        return containsAnyWord(line, kWords, std::size(kWords));
    }

    bool looksLikeSuccess(std::string_view line) {
        static constexpr std::string_view kWords[] = {
            "success", "successful", "completed", "connected", "ready", "saved",
            "applied", "loaded", "initialized", "exported", "copied", "cleaned",
            "started", "opened",
        };
        return containsAnyWord(line, kWords, std::size(kWords));
    }

}
