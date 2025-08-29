#include <windows.h>
#include <string>
#include "nfd.h"
#include "logger/logger.h"
#include "application.h"

namespace Application {
    void openZmxFileInZemax() {
        nfdchar_t* outPath = nullptr;
        nfdresult_t result = NFD_OpenDialog("zmx", nullptr, &outPath);

        if (result == NFD_OKAY) {
            // UTF-8 → UTF-16
            int size = MultiByteToWideChar(CP_UTF8, 0, outPath, -1, nullptr, 0);
            std::wstring widePath(size, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, outPath, -1, widePath.data(), size);

            HINSTANCE ret = ShellExecuteW(nullptr, L"open", widePath.c_str(), nullptr, nullptr, SW_SHOW);
            if ((intptr_t)ret <= 32) {
                MessageBoxW(nullptr, L"Failed to open file. Is Zemax installed?", L"Error", MB_ICONERROR);
            }

            free(outPath);
        } else if (result == NFD_CANCEL) {
            // Do nothing
        } else {
            const char* error = NFD_GetError();
            logger.addLog("NFD Error: " + std::string(error ? error : "Unknown"));
        }
    }
} // namespace Application