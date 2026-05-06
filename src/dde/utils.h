#pragma once

#include <windows.h>
#include <string>
#include <string_view>
#include <vector>

namespace ZemaxDDE {
    std::vector<std::string> tokenize(std::string_view bufferStr);
    std::string cp1251_to_utf8(const char* data, size_t len);
    std::string extractStringFromDDE(GLOBALHANDLE handle);
}
