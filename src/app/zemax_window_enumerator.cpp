#include "zemax_window_enumerator.h"

#include <tlhelp32.h>

namespace app {
    struct EnumWindowContext {
        DWORD targetPid;
        std::vector<ZemaxWindowInfo>& results;
    };

    static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam) {
        auto& ctx = *reinterpret_cast<EnumWindowContext*>(lParam);

        DWORD windowPid = 0;
        GetWindowThreadProcessId(hwnd, &windowPid);

        if (windowPid != ctx.targetPid) {
            return TRUE;
        }

        if (!IsWindowVisible(hwnd)) {
            return TRUE;
        }

        if (GetWindow(hwnd, GW_OWNER) != nullptr) {
            return TRUE;
        }

        if (GetParent(hwnd) != nullptr) {
            return TRUE;
        }

        if (GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_CHILD) {
            return TRUE;
        }

        wchar_t title[256];
        int len = GetWindowTextW(hwnd, title, sizeof(title) / sizeof(title[0]));
        if (len == 0) {
            return TRUE;
        }

        ZemaxWindowInfo info;
        info.pid = ctx.targetPid;
        info.hwnd = hwnd;
        info.title.assign(title, static_cast<size_t>(len));
        ctx.results.push_back(std::move(info));

        return TRUE;
    }

    static void enumerateWindowsForPid(DWORD pid, std::vector<ZemaxWindowInfo>& results) {
        EnumWindowContext ctx{pid, results};
        EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&ctx));
    }

    std::vector<ZemaxWindowInfo> ZemaxWindowEnumerator::enumerate() {
        std::vector<ZemaxWindowInfo> results;

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return results;
        }

        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);

        if (Process32FirstW(snapshot, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"zemax.exe") == 0) {
                    enumerateWindowsForPid(pe.th32ProcessID, results);
                }
            } while (Process32NextW(snapshot, &pe));
        }

        CloseHandle(snapshot);
        return results;
    }
}
