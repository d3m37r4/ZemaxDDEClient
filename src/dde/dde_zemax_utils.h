#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ZemaxDDE {
    std::vector<std::string> tokenize(std::string_view bufferStr);
}
