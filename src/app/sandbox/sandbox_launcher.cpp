#include "app/sandbox/sandbox_launcher.h"

#include <cstring>
#include <filesystem>
#include <tlhelp32.h>

#include "app/config_path.h"
#include "logger/logger.h"

namespace app::sandbox {
    namespace {
        // Keep in sync with src/app/sandbox/hook/sandbox.rc.in.
        constexpr WORD kHookResourceId = 101;
        constexpr const wchar_t* kHookResourceType = L"BINARY";
        constexpr const wchar_t* kHookDllName = L"SandboxHook.dll";

        bool Utf8ToWide(const std::string& in, std::wstring& out) {
            if (in.empty()) {
                out.clear();
                return true;
            }
            const int needed =
                MultiByteToWideChar(CP_UTF8, 0, in.c_str(), -1, nullptr, 0);
            if (needed <= 1) {
                return false;
            }
            out.resize(static_cast<size_t>(needed) - 1);
            return MultiByteToWideChar(CP_UTF8, 0, in.c_str(), -1, out.data(), needed) > 0;
        }

        std::wstring GetSettingsDir() {
            std::wstring wide;
            if (!Utf8ToWide(app::getSettingsJsonPath(), wide) || wide.empty()) {
                return std::filesystem::current_path().wstring();
            }
            std::filesystem::path dir = std::filesystem::path(wide).parent_path();
            if (dir.empty()) {
                dir = std::filesystem::current_path();
            }
            return dir.wstring();
        }

        bool LoadHookFromResource(const BYTE*& data, DWORD& size) {
            HRSRC res = FindResourceW(nullptr, MAKEINTRESOURCEW(kHookResourceId), kHookResourceType);
            if (res == nullptr) {
                return false;
            }
            HGLOBAL handle = LoadResource(nullptr, res);
            if (handle == nullptr) {
                return false;
            }
            size = SizeofResource(nullptr, res);
            data = static_cast<const BYTE*>(LockResource(handle));
            return data != nullptr && size != 0;
        }

        bool GetExeDirHookPath(std::wstring& outPath) {
            wchar_t exePath[MAX_PATH * 2]{};
            DWORD len = GetModuleFileNameW(nullptr, exePath, static_cast<DWORD>(sizeof(exePath) / sizeof(exePath[0])));
            if (len == 0) {
                return false;
            }
            std::wstring dir(exePath, len);
            const size_t sep = dir.find_last_of(L"\\/");
            if (sep != std::wstring::npos) {
                dir.resize(sep + 1);
            }
            outPath = dir + kHookDllName;
            const DWORD attrs = GetFileAttributesW(outPath.c_str());
            return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
        }

        bool InjectHookDll(HANDLE process, const std::wstring& dllPath, std::string& errorOut) {
            const size_t bytes = (dllPath.size() + 1) * sizeof(wchar_t);
            LPVOID remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (remote == nullptr) {
                errorOut = "VirtualAllocEx failed, error " + std::to_string(GetLastError());
                return false;
            }
            if (!WriteProcessMemory(process, remote, dllPath.c_str(), bytes, nullptr)) {
                errorOut = "WriteProcessMemory failed, error " + std::to_string(GetLastError());
                VirtualFreeEx(process, remote, 0, MEM_RELEASE);
                return false;
            }
            HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
            FARPROC loadLibraryAddr = GetProcAddress(kernel32, "LoadLibraryW");
            LPTHREAD_START_ROUTINE loadLibrary = nullptr;
            static_assert(sizeof(loadLibrary) == sizeof(loadLibraryAddr));
            memcpy(&loadLibrary, &loadLibraryAddr, sizeof(loadLibrary));
            if (loadLibrary == nullptr) {
                errorOut = "GetProcAddress(LoadLibraryW) failed";
                VirtualFreeEx(process, remote, 0, MEM_RELEASE);
                return false;
            }
            HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
            if (thread == nullptr) {
                const DWORD err = GetLastError();
                VirtualFreeEx(process, remote, 0, MEM_RELEASE);
                if (err == ERROR_ACCESS_DENIED) {
                    errorOut = "CreateRemoteThread denied (bitness mismatch? hook is "
                               + std::string(sizeof(void*) == 8 ? "64-bit" : "32-bit") + ")";
                } else {
                    errorOut = "CreateRemoteThread failed, error " + std::to_string(err);
                }
                return false;
            }
            WaitForSingleObject(thread, 10000);
            DWORD exitCode = 0;
            GetExitCodeThread(thread, &exitCode);
            CloseHandle(thread);
            VirtualFreeEx(process, remote, 0, MEM_RELEASE);
            if (exitCode == 0) {
                errorOut = "Hook DLL failed to load in target (LoadLibrary returned NULL)";
                return false;
            }
            return true;
        }
    }

    bool ResolveZemaxExePath(std::wstring& outPath) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD zemaxPid = 0;
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snapshot, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"zemax.exe") == 0) {
                    zemaxPid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &pe));
        }
        CloseHandle(snapshot);

        if (zemaxPid == 0) {
            return false;
        }

        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, zemaxPid);
        if (process == nullptr) {
            return false;
        }

        wchar_t buffer[MAX_PATH * 2];
        DWORD size = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
        const BOOL ok = QueryFullProcessImageNameW(process, 0, buffer, &size);
        CloseHandle(process);

        if (!ok || size == 0) {
            return false;
        }
        outPath.assign(buffer, size);
        return true;
    }

    bool EnsureHookDeployed(Logger& logger, std::wstring& outPath, std::string& error) {
        std::filesystem::path dir(GetSettingsDir());
#ifdef APP_PRODUCTION_BUILD
        dir /= L"modules";
#endif
        const std::wstring target = (dir / kHookDllName).wstring();

        const BYTE* data = nullptr;
        DWORD size = 0;
        if (!LoadHookFromResource(data, size)) {
            // No embedded resource (should not happen in release builds):
            // fall back to a DLL shipped next to the exe.
            if (GetExeDirHookPath(outPath)) {
                logger.addLog("[SANDBOX] No embedded hook, using exe-dir copy");
                return true;
            }
            error = "No embedded hook resource and no SandboxHook.dll next to the app exe";
            logger.addLog("[SANDBOX] Deploy failed: " + error);
            return false;
        }

        bool write = true;
        WIN32_FILE_ATTRIBUTE_DATA attrs{};
        if (GetFileAttributesExW(target.c_str(), GetFileExInfoStandard, &attrs)) {
            const ULONGLONG existing =
                (static_cast<ULONGLONG>(attrs.nFileSizeHigh) << 32) | attrs.nFileSizeLow;
            write = existing != size;
        } else {
            std::error_code ec;
            std::filesystem::create_directories(std::filesystem::path(target).parent_path(), ec);
        }

        if (write) {
            HANDLE file = CreateFileW(target.c_str(), GENERIC_WRITE, 0, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE) {
                const DWORD err = GetLastError();
                if (err == ERROR_SHARING_VIOLATION || err == ERROR_ACCESS_DENIED) {
                    // A previous copy is mapped into a live process: reuse it,
                    // it was deployed from this same resource earlier.
                    logger.addLog("[SANDBOX] Hook file locked, reusing existing copy");
                } else {
                    error = "Cannot write hook DLL, error " + std::to_string(err);
                    logger.addLog("[SANDBOX] Deploy failed: " + error);
                    return false;
                }
            } else {
                DWORD written = 0;
                const BOOL ok = WriteFile(file, data, size, &written, nullptr);
                CloseHandle(file);
                if (!ok || written != size) {
                    error = "Cannot write hook DLL (short write)";
                    logger.addLog("[SANDBOX] Deploy failed: " + error);
                    return false;
                }
                logger.addLog("[SANDBOX] Hook DLL deployed, " + std::to_string(size) + " bytes");
            }
        }

        outPath = target;
        return true;
    }

    LaunchResult LaunchSandboxedZemax(Logger& logger) {
        LaunchResult result;

        std::wstring exePath;
        if (!ResolveZemaxExePath(exePath)) {
            result.error = "Zemax is not running — launch Zemax normally first";
            logger.addLog("[SANDBOX] Launch failed: no running zemax.exe found");
            return result;
        }

        std::wstring hookPath;
        std::string deployError;
        if (!EnsureHookDeployed(logger, hookPath, deployError)) {
            result.error = deployError;
            return result;
        }

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        if (!CreateProcessW(exePath.c_str(), nullptr, nullptr, nullptr, FALSE,
                            CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS,
                            nullptr, nullptr, &si, &pi)) {
            const DWORD err = GetLastError();
            result.error = "CreateProcess failed, error " + std::to_string(err);
            logger.addLog("[SANDBOX] CreateProcess failed, error " + std::to_string(err));
            return result;
        }

        std::string injectError;
        if (!InjectHookDll(pi.hProcess, hookPath, injectError)) {
            logger.addLog("[SANDBOX] Injection failed: " + injectError + " — resuming without hook");
            // Resume anyway so we never leave a frozen process behind.
            ResumeThread(pi.hThread);
            result.error = injectError;
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return result;
        }

        ResumeThread(pi.hThread);
        result.ok = true;
        result.pid = pi.dwProcessId;
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        logger.addLog("[SANDBOX] Launched sandboxed zemax.exe, pid " + std::to_string(pi.dwProcessId));
        return result;
    }
}
