#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace app {
    struct ZemaxWindowInfo {
        DWORD pid;
        HWND hwnd;
        std::wstring title;
    };

    class ZemaxWindowEnumerator {
    public:
        static std::vector<ZemaxWindowInfo> enumerate();
    };
}
