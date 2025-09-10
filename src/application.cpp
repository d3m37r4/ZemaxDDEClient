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
        #ifdef DEBUG_LOG
            logger.addLog("[APP] Selected file: " + std::string(outPath));
        #endif
            // UTF-8 → UTF-16
            int size = MultiByteToWideChar(CP_UTF8, 0, outPath, -1, nullptr, 0);
            std::wstring widePath(size, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, outPath, -1, widePath.data(), size);

            HINSTANCE ret = ShellExecuteW(nullptr, L"open", widePath.c_str(), nullptr, nullptr, SW_SHOW);
            if ((intptr_t)ret <= 32) {
                MessageBoxW(nullptr, L"Failed to open file. Is Zemax installed?", L"Error", MB_ICONERROR);
                logger.addLog("[APP] ShellExecute failed to open file: " + std::string(outPath) + 
                            " (Error code: " + std::to_string((intptr_t)ret) + ")");
            } else {
            #ifdef DEBUG_LOG
                logger.addLog("[APP] Successfully sent file to ShellExecute: " + std::string(outPath));
            #endif
            }

            free(outPath);
        } else if (result == NFD_CANCEL) {
        #ifdef DEBUG_LOG
            logger.addLog("[APP] File open dialog canceled by user");
        #endif
        } else {
            const char* error = NFD_GetError();
            logger.addLog("[APP] NFD Error: " + std::string(error ? error : "Unknown error"));
        }
    }
} // namespace Application
