// SandboxHook: hides foreign "wzemax"/"wzemax2" top windows from the host
// process so Zemax 13 skips its "2 Copies are already running!" guard.
// Pure WinAPI, no third-party deps. IAT patch of user32 imports.

#include <windows.h>

// ---------------------------------------------------------------- originals
static HWND(WINAPI* g_realFindWindowW)(LPCWSTR, LPCWSTR) = nullptr;
static HWND(WINAPI* g_realFindWindowA)(LPCSTR, LPCSTR) = nullptr;
static HWND(WINAPI* g_realFindWindowExW)(HWND, HWND, LPCWSTR, LPCWSTR) = nullptr;
static HWND(WINAPI* g_realFindWindowExA)(HWND, HWND, LPCSTR, LPCSTR) = nullptr;
static BOOL(WINAPI* g_realEnumWindows)(WNDENUMPROC, LPARAM) = nullptr;

// ---------------------------------------------------------------- helpers
static bool IsHiddenClassW(LPCWSTR cls) {
    if (cls == nullptr) {
        return false;
    }
    if ((((ULONG_PTR)cls) >> 16) == 0) {
        return false; // atom, not a name
    }
    return _wcsicmp(cls, L"wzemax") == 0 || _wcsicmp(cls, L"wzemax2") == 0;
}

static bool IsHiddenClassA(LPCSTR cls) {
    if (cls == nullptr) {
        return false;
    }
    if ((((ULONG_PTR)cls) >> 16) == 0) {
        return false; // atom, not a name
    }
    return _stricmp(cls, "wzemax") == 0 || _stricmp(cls, "wzemax2") == 0;
}

// ---------------------------------------------------------------- hooks
static HWND WINAPI HookFindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName) {
    if (IsHiddenClassW(lpClassName)) {
        return nullptr; // pretend no other Zemax copy exists
    }
    return g_realFindWindowW(lpClassName, lpWindowName);
}

static HWND WINAPI HookFindWindowA(LPCSTR lpClassName, LPCSTR lpWindowName) {
    if (IsHiddenClassA(lpClassName)) {
        return nullptr;
    }
    return g_realFindWindowA(lpClassName, lpWindowName);
}

static HWND WINAPI HookFindWindowExW(HWND hParent, HWND hChildAfter, LPCWSTR lpClassName, LPCWSTR lpTitle) {
    if (hParent == nullptr && IsHiddenClassW(lpClassName)) {
        return nullptr;
    }
    return g_realFindWindowExW(hParent, hChildAfter, lpClassName, lpTitle);
}

static HWND WINAPI HookFindWindowExA(HWND hParent, HWND hChildAfter, LPCSTR lpClassName, LPCSTR lpTitle) {
    if (hParent == nullptr && IsHiddenClassA(lpClassName)) {
        return nullptr;
    }
    return g_realFindWindowExA(hParent, hChildAfter, lpClassName, lpTitle);
}

struct EnumFilterCtx {
    WNDENUMPROC userCallback;
    LPARAM userParam;
    DWORD selfPid;
};

static BOOL CALLBACK EnumFilterProc(HWND hwnd, LPARAM lParam) {
    const auto* ctx = reinterpret_cast<EnumFilterCtx*>(lParam);
    wchar_t cls[64]{};
    if (GetClassNameW(hwnd, cls, static_cast<int>(sizeof(cls) / sizeof(cls[0]))) > 0 && IsHiddenClassW(cls)) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid != ctx->selfPid) {
            return TRUE; // skip foreign Zemax windows, keep enumerating
        }
    }
    return ctx->userCallback(hwnd, ctx->userParam);
}

static BOOL WINAPI HookEnumWindows(WNDENUMPROC lpEnumFunc, LPARAM lParam) {
    if (lpEnumFunc == nullptr) {
        return FALSE;
    }
    EnumFilterCtx ctx{lpEnumFunc, lParam, GetCurrentProcessId()};
    return g_realEnumWindows(EnumFilterProc, reinterpret_cast<LPARAM>(&ctx));
}

// ---------------------------------------------------------------- IAT patch
static bool PatchIATEntry(const char* targetDll, const char* funcName, void* newFunc, void** oldFunc) {
    const HMODULE exe = GetModuleHandleW(nullptr);
    if (exe == nullptr) {
        return false;
    }
    HMODULE dll = GetModuleHandleA(targetDll);
    if (dll == nullptr) {
        dll = LoadLibraryA(targetDll);
        if (dll == nullptr) {
            return false;
        }
    }
    const auto target = reinterpret_cast<void*>(GetProcAddress(dll, funcName));
    if (target == nullptr) {
        return false;
    }

    auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(exe);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }
    auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<BYTE*>(exe) + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }
    const auto& imports = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }
    auto* desc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(reinterpret_cast<BYTE*>(exe) + imports.VirtualAddress);
    for (; desc->Name != 0; ++desc) {
        const char* name = reinterpret_cast<const char*>(reinterpret_cast<BYTE*>(exe) + desc->Name);
        if (_stricmp(name, targetDll) != 0) {
            continue;
        }
        auto* thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(reinterpret_cast<BYTE*>(exe) + desc->FirstThunk);
        for (; thunk->u1.Function != 0; ++thunk) {
            auto** funcPtr = reinterpret_cast<void**>(&thunk->u1.Function);
            if (*funcPtr != target) {
                continue;
            }
            DWORD oldProtect = 0;
            if (!VirtualProtect(funcPtr, sizeof(void*), PAGE_READWRITE, &oldProtect)) {
                return false;
            }
            *oldFunc = *funcPtr;
            *funcPtr = newFunc;
            VirtualProtect(funcPtr, sizeof(void*), oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
            return true;
        }
    }
    return false;
}

static void InstallHooks() {
    PatchIATEntry("user32.dll", "FindWindowW", reinterpret_cast<void*>(HookFindWindowW),
                  reinterpret_cast<void**>(&g_realFindWindowW));
    PatchIATEntry("user32.dll", "FindWindowA", reinterpret_cast<void*>(HookFindWindowA),
                  reinterpret_cast<void**>(&g_realFindWindowA));
    PatchIATEntry("user32.dll", "FindWindowExW", reinterpret_cast<void*>(HookFindWindowExW),
                  reinterpret_cast<void**>(&g_realFindWindowExW));
    PatchIATEntry("user32.dll", "FindWindowExA", reinterpret_cast<void*>(HookFindWindowExA),
                  reinterpret_cast<void**>(&g_realFindWindowExA));
    PatchIATEntry("user32.dll", "EnumWindows", reinterpret_cast<void*>(HookEnumWindows),
                  reinterpret_cast<void**>(&g_realEnumWindows));
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID /*reserved*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        InstallHooks();
    }
    return TRUE;
}
