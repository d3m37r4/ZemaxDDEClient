#pragma once

#include <string>
#include <windows.h>

class Logger;

namespace app::sandbox {
    struct LaunchResult {
        bool ok = false;
        DWORD pid = 0;
        std::string error;
    };

    /// Resolves zemax.exe path from a running instance (via process snapshot).
    /// Returns false when no Zemax process is running.
    bool ResolveZemaxExePath(std::wstring& outPath);

    /// Extracts the embedded hook DLL into the settings folder when missing
    /// or stale (single-file distribution: the DLL ships inside the exe).
    /// Falls back to a DLL next to the exe when no resource is embedded.
    /// Returns false + error on failure.
    bool EnsureHookDeployed(Logger& logger, std::wstring& outPath, std::string& error);

    /// Launches another Zemax copy with the hook injected, bypassing the
    /// vendor "2 Copies are already running!" guard.
    LaunchResult LaunchSandboxedZemax(Logger& logger);
}
