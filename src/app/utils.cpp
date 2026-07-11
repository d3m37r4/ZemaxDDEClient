#include "app/utils.h"

#include <cstdio>
#include <string>

namespace app {
    std::string formatFileSize(uintmax_t bytes) {
        if (bytes < 1024)               return std::to_string(bytes) + " B";
        if (bytes < 1024 * 1024)        return std::to_string(bytes / 1024) + " KB";
        char buf[32];
        if (bytes < 1024 * 1024 * 1024) {
            std::snprintf(buf, sizeof(buf), "%.2f MB", static_cast<double>(bytes) / (1024 * 1024));
            return buf;
        }
        std::snprintf(buf, sizeof(buf), "%.2f GB", static_cast<double>(bytes) / (1024 * 1024 * 1024));
        return buf;
    }
}
