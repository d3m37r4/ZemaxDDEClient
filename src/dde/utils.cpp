#include "utils.h"
#include <format>
#include <windows.h>

namespace ZemaxDDE {
    // Extract string payload from a DDE data handle (skip 4‑byte header, trim \r\n\0, convert CP1251→UTF‑8)
    std::string extractStringFromDDE(GLOBALHANDLE handle) {
        if (!handle) return {};
        HGLOBAL h = static_cast<HGLOBAL>(handle);
        LPBYTE pData = static_cast<LPBYTE>(GlobalLock(h));
        if (!pData) return {};

        SIZE_T totalSize = GlobalSize(h);
        constexpr SIZE_T DDE_HEADER_SIZE = 4;
        if (totalSize <= DDE_HEADER_SIZE) {
            GlobalUnlock(h);
            return {};
        }

        const char* raw = reinterpret_cast<const char*>(pData + DDE_HEADER_SIZE);
        size_t len = totalSize - DDE_HEADER_SIZE;
        while (len && (raw[len - 1] == '\0' || raw[len - 1] == '\r' || raw[len - 1] == '\n')) {
            --len;
        }

        std::string result = cp1251_to_utf8(raw, len);
        GlobalUnlock(h);
        return result;
    }

    // Convert CP1251 (Windows-1251) encoded data to UTF‑8
    std::string cp1251_to_utf8(const char* data, size_t len) {
        if (!data || len == 0) return {};
        int wlen = MultiByteToWideChar(1251, 0, data, static_cast<int>(len), nullptr, 0);
        
        if (wlen <= 0) return {};

        std::wstring wstr(wlen, L'\0');
        MultiByteToWideChar(1251, 0, data, static_cast<int>(len), &wstr[0], wlen);

        int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, nullptr, 0, nullptr, nullptr);
        if (u8len <= 0) return {};

        std::string u8str(u8len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, &u8str[0], u8len, nullptr, nullptr);

        return u8str;
    }

    // Convert UTF‑16 (wchar_t) string to UTF‑8
    std::string wstring_to_utf8(const std::wstring& wstr) {
        if (wstr.empty()) return {};
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(),
            static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        if (len <= 0) return {};
        std::string result(static_cast<size_t>(len), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(),
            static_cast<int>(wstr.size()), result.data(), len, nullptr, nullptr);
        return result;
    }

    /*
    * Splits the input string into individual "tokens" (meaningful parts).
    * Tokens are delimited by commas, spaces, newlines, or carriage returns, unless they are enclosed within double quotes.
    * Returns a vector of strings, where each string is a token.
    */
    std::vector<std::string> tokenize(std::string_view bufferStr) {
        std::vector<std::string> tokens;
        std::string currentToken;
        bool inQuotes = false;

        for (size_t i = 0; i < bufferStr.size(); ++i) {
            char c = bufferStr[i];
            // Handle escaped quotes: if we see a backslash followed by a quote, skip the backslash and add the quote literally
            if (c == '\\' && i + 1 < bufferStr.size() && bufferStr[i + 1] == '"') {
                currentToken += '"';
                ++i; // skip the quote character
                continue;
            }
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if ((c == ',' || c == '\n' || c == '\r' || c == ' ') && !inQuotes) {
                if (!currentToken.empty()) {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
            } else {
                currentToken += c;
            }
        }

        if (!currentToken.empty()) {
            tokens.push_back(currentToken);
        }

        return tokens;
    }

    std::string formatDuration(std::chrono::milliseconds dur) {
        auto ms = dur.count();
        auto hours = ms / 3'600'000; ms %= 3'600'000;
        auto mins  = ms / 60'000;    ms %= 60'000;
        auto secs  = ms / 1000.0;

        if (hours > 0)
            return std::format("{}h {}m {:.3f}s", hours, mins, secs);
        if (mins > 0)
            return std::format("{}m {:.3f}s", mins, secs);
        return std::format("{:.3f}s", secs);
    }
}
