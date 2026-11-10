#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>

namespace ZemaxDDE {
    std::vector<std::string> tokenize(std::string_view bufferStr);
    std::string cp1251_to_utf8(const char* data, size_t len);
    std::string wstring_to_utf8(const std::wstring& wstr);
    /// Formats a duration for human-readable logging.
    /// Examples: "2.345s", "10m 30.456s", "1h 2m 3.456s"
    std::string formatDuration(std::chrono::milliseconds dur);

    /// Reply of the GetRefresh/GetUpdate item.
    enum class UpdateStatus { Updated, NotUpdated, Unknown };

    /// "0" -> Updated, "-1" -> NotUpdated, anything else (empty, timeout text,
    /// garbage) -> Unknown. Trims NUL/CR/LF/spaces the DDE payload may carry.
    UpdateStatus parseUpdateStatus(std::string_view buffer);
}
