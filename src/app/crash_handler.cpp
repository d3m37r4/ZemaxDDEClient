#include "app/crash_handler.h"

#include "app/config_path.h"
#include "version.h"

#include <windows.h>
#include <tlhelp32.h>
#include <malloc.h>
#include <csignal>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <exception>
#include <stdexcept>

namespace app::crash {
namespace {

    /// C++ exceptions are implemented with SEH on MinGW, so every throw
    /// reaches the vectored handler with this code. It is not a crash.
    constexpr DWORD kCppExceptionCode = 0xE06D7363;

    constexpr DWORD kStatusStackBufferOverrun = 0xC0000409;
    constexpr DWORD kStatusHeapCorruption = 0xC0000379;

    constexpr size_t kReportCapacity = 64 * 1024;
    constexpr int kMaxModules = 192;
    constexpr DWORD kMaxFrames = 62;

    std::atomic<bool> g_handling{false};

    const Logger* g_logger = nullptr;

    struct ModuleInfo {
        char name[MAX_PATH];
        uintptr_t base;
        size_t size;
    };
    ModuleInfo g_modules[kMaxModules];
    int g_moduleCount = 0;

    void* g_frames[kMaxFrames];
    DWORD g_frameCount = 0;

    char g_report[kReportCapacity];
    size_t g_reportLength = 0;

    /// A crash handler runs in a broken process: the heap may be corrupted and
    /// another thread may hold a lock. Everything below therefore avoids
    /// allocations, locks and library calls that are not plain Win32.
    void appendText(const char* text) {
        if (text == nullptr) {
            return;
        }
        const size_t length = std::strlen(text);
        if (g_reportLength + length + 1 > kReportCapacity) {
            return;
        }
        std::memcpy(g_report + g_reportLength, text, length + 1);
        g_reportLength += length;
    }

    void appendFormat(const char* format, ...) {
        char buffer[1024];
        va_list arguments;
        va_start(arguments, format);
        const int written = std::vsnprintf(buffer, sizeof(buffer), format, arguments);
        va_end(arguments);
        if (written > 0) {
            appendText(buffer);
        }
    }

    void appendWide(wchar_t* destination, size_t capacity, const wchar_t* source) {
        size_t index = 0;
        while (destination[index] != 0 && index + 1 < capacity) {
            ++index;
        }
        while (*source != 0 && index + 1 < capacity) {
            destination[index++] = *source++;
        }
        destination[index] = 0;
    }

    const char* exceptionName(DWORD code) {
        switch (code) {
            case EXCEPTION_ACCESS_VIOLATION: return "ACCESS_VIOLATION";
            case EXCEPTION_STACK_OVERFLOW: return "STACK_OVERFLOW";
            case EXCEPTION_INT_DIVIDE_BY_ZERO: return "INT_DIVIDE_BY_ZERO";
            case EXCEPTION_ILLEGAL_INSTRUCTION: return "ILLEGAL_INSTRUCTION";
            case EXCEPTION_PRIV_INSTRUCTION: return "PRIV_INSTRUCTION";
            case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "FLT_DIVIDE_BY_ZERO";
            case EXCEPTION_FLT_OVERFLOW: return "FLT_OVERFLOW";
            case EXCEPTION_IN_PAGE_ERROR: return "IN_PAGE_ERROR";
            case EXCEPTION_DATATYPE_MISALIGNMENT: return "DATATYPE_MISALIGNMENT";
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "ARRAY_BOUNDS_EXCEEDED";
            case kStatusStackBufferOverrun: return "STACK_BUFFER_OVERRUN";
            case kStatusHeapCorruption: return "HEAP_CORRUPTION";
            case kCppExceptionCode: return "CPP_EXCEPTION";
            default: return "UNKNOWN";
        }
    }

    void collectModules() {
        g_moduleCount = 0;

        const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
        if (snapshot == INVALID_HANDLE_VALUE) {
            return;
        }

        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (Module32FirstW(snapshot, &entry)) {
            do {
                if (g_moduleCount >= kMaxModules) {
                    break;
                }
                ModuleInfo& info = g_modules[g_moduleCount];
                if (WideCharToMultiByte(CP_UTF8, 0, entry.szModule, -1, info.name, MAX_PATH, nullptr, nullptr) == 0) {
                    info.name[0] = '\0';
                }
                info.base = reinterpret_cast<uintptr_t>(entry.modBaseAddr);
                info.size = entry.modBaseSize;
                ++g_moduleCount;
            } while (Module32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }

    const ModuleInfo* findModule(uintptr_t address) {
        for (int i = 0; i < g_moduleCount; ++i) {
            if (address >= g_modules[i].base && address < g_modules[i].base + g_modules[i].size) {
                return &g_modules[i];
            }
        }
        return nullptr;
    }

    void captureStack() {
        g_frameCount = CaptureStackBackTrace(0, kMaxFrames, g_frames, nullptr);
    }

    void appendContext(const CONTEXT* context) {
        if (context == nullptr) {
            return;
        }
#if defined(__x86_64__)
        appendFormat("\nCONTEXT        : RIP=0x%016llX RSP=0x%016llX RBP=0x%016llX\n",
                     static_cast<unsigned long long>(context->Rip),
                     static_cast<unsigned long long>(context->Rsp),
                     static_cast<unsigned long long>(context->Rbp));
        appendFormat("                  RAX=0x%016llX RBX=0x%016llX RCX=0x%016llX RDX=0x%016llX\n",
                     static_cast<unsigned long long>(context->Rax),
                     static_cast<unsigned long long>(context->Rbx),
                     static_cast<unsigned long long>(context->Rcx),
                     static_cast<unsigned long long>(context->Rdx));
        appendFormat("                  RSI=0x%016llX RDI=0x%016llX R8=0x%016llX R9=0x%016llX\n",
                     static_cast<unsigned long long>(context->Rsi),
                     static_cast<unsigned long long>(context->Rdi),
                     static_cast<unsigned long long>(context->R8),
                     static_cast<unsigned long long>(context->R9));
#else
        appendFormat("\nCONTEXT        : EIP=0x%08lX ESP=0x%08lX EBP=0x%08lX\n",
                     static_cast<unsigned long long>(context->Eip),
                     static_cast<unsigned long long>(context->Esp),
                     static_cast<unsigned long long>(context->Ebp));
#endif
    }

    void appendModules() {
        appendFormat("\nModules (name  base  size):\n");
        for (int i = 0; i < g_moduleCount; ++i) {
            appendFormat("  %-30s 0x%016llX  0x%08X\n", g_modules[i].name,
                         static_cast<unsigned long long>(g_modules[i].base),
                         static_cast<unsigned>(g_modules[i].size));
        }
    }

    void appendStack() {
        if (g_frameCount == 0) {
            return;
        }
        appendFormat("\nStack (module+offset):\n");
        for (DWORD i = 0; i < g_frameCount; ++i) {
            const uintptr_t address = reinterpret_cast<uintptr_t>(g_frames[i]);
            const ModuleInfo* module = findModule(address);
            if (module != nullptr) {
                appendFormat("  #%-2u %s + 0x%llX\n", i, module->name,
                             static_cast<unsigned long long>(address - module->base));
            } else {
                appendFormat("  #%-2u 0x%016llX\n", i, static_cast<unsigned long long>(address));
            }
        }
    }

    void buildReport(const char* reason, EXCEPTION_POINTERS* pointers, bool includeDetails) {
        SYSTEMTIME time{};
        GetLocalTime(&time);

        if (includeDetails) {
            collectModules();
            captureStack();
        }

        appendFormat("=== ZemaxDDEClient crash report ===\n");
        appendFormat("Version : %s (build %s) commit %s\n",
                     APP_VERSION_STRING, TOSTRING(APP_BUILD_NUMBER), APP_GIT_COMMIT);
        appendFormat("Time    : %04d-%02d-%02d %02d:%02d:%02d\n",
                     time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
        appendFormat("PID/TID : %lu / %lu\n",
                     GetCurrentProcessId(), GetCurrentThreadId());
        appendFormat("Reason  : %s\n", reason);

        if (pointers != nullptr && pointers->ExceptionRecord != nullptr) {
            const EXCEPTION_RECORD* record = pointers->ExceptionRecord;
            appendFormat("\nException code : 0x%08lX (%s)\n",
                         record->ExceptionCode, exceptionName(record->ExceptionCode));

            const uintptr_t faultAddress = reinterpret_cast<uintptr_t>(record->ExceptionAddress);
            const ModuleInfo* faultModule = includeDetails ? findModule(faultAddress) : nullptr;
            if (faultModule != nullptr) {
                appendFormat("Fault address  : 0x%016llX  (%s + 0x%llX)\n",
                             static_cast<unsigned long long>(faultAddress), faultModule->name,
                             static_cast<unsigned long long>(faultAddress - faultModule->base));
            } else {
                appendFormat("Fault address  : 0x%016llX\n", static_cast<unsigned long long>(faultAddress));
            }

            if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
                appendFormat("Access type    : %s\n", record->ExceptionInformation[0] != 0 ? "write" : "read");
                appendFormat("Faulting addr  : 0x%016llX\n",
                             static_cast<unsigned long long>(record->ExceptionInformation[1]));
            }
        }

        if (includeDetails) {
            appendContext(pointers != nullptr ? pointers->ContextRecord : nullptr);
            appendModules();
            appendStack();
        }
    }

    /// Builds the path of the crash report. The reports live in a "crash"
    /// subfolder so that the "Clean logs" action does not delete them.
    void buildReportPath(wchar_t* path, size_t capacity, const wchar_t* extension) {
        path[0] = 0;

        const char* logFolder = app::getLogFolderPath();
        if (logFolder != nullptr && logFolder[0] != '\0') {
            MultiByteToWideChar(CP_UTF8, 0, logFolder, -1, path, static_cast<int>(capacity));
        }
        appendWide(path, capacity, L"\\crash");
        CreateDirectoryW(path, nullptr);

        SYSTEMTIME time{};
        GetLocalTime(&time);
        wchar_t fileName[MAX_PATH] = {};
        swprintf(fileName, MAX_PATH, L"crash-%04d%02d%02d-%02d%02d%02d-%lu%ls",
                 time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond,
                 GetCurrentProcessId(), extension);

        appendWide(path, capacity, L"\\");
        appendWide(path, capacity, fileName);
    }

    void writeReportFile() {
        wchar_t path[MAX_PATH] = {};
        buildReportPath(path, MAX_PATH, L".log");

        const HANDLE file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            return;
        }
        DWORD written = 0;
        WriteFile(file, g_report, static_cast<DWORD>(g_reportLength), &written, nullptr);
        CloseHandle(file);
    }

    bool isHandling() {
        return g_handling.load(std::memory_order_acquire);
    }

    void beginHandling() {
        bool expected = false;
        g_handling.compare_exchange_strong(expected, true, std::memory_order_acq_rel);
    }

    LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* pointers) {
        if (isHandling()) {
            return EXCEPTION_CONTINUE_SEARCH;
        }
        beginHandling();

        buildReport("unhandled exception", pointers, true);
        writeReportFile();

        // Let the system handle it: the user still gets the usual dialog.
        return EXCEPTION_CONTINUE_SEARCH;
    }

    LONG CALLBACK vectoredExceptionHandler(EXCEPTION_POINTERS* pointers) {
        if (pointers == nullptr || pointers->ExceptionRecord == nullptr) {
            return EXCEPTION_CONTINUE_SEARCH;
        }
        if (pointers->ExceptionRecord->ExceptionCode == kCppExceptionCode) {
            // Every C++ throw passes through SEH on MinGW: not a crash.
            return EXCEPTION_CONTINUE_SEARCH;
        }
        if (pointers->ExceptionRecord->ExceptionCode != EXCEPTION_STACK_OVERFLOW) {
            return EXCEPTION_CONTINUE_SEARCH;
        }
        if (isHandling()) {
            return EXCEPTION_CONTINUE_SEARCH;
        }
        beginHandling();

        // The stack is exhausted: keep the report minimal, the deep calls a
        // stack capture needs would fault again.
        buildReport("stack overflow", pointers, false);
        writeReportFile();

        return EXCEPTION_CONTINUE_SEARCH;
    }

    void signalAbortHandler(int signalNumber) {
        if (!isHandling()) {
            beginHandling();
            // abort() does not go through std::terminate, so the terminate
            // handler never sees it. assert() failures end up here as well.
            buildReport("abort()", nullptr, true);
            writeReportFile();
        }
        // Hand the signal back to the CRT so the process dies as expected.
        std::signal(signalNumber, SIG_DFL);
        std::raise(signalNumber);
    }

    void terminateHandler() {
        if (isHandling()) {
            TerminateProcess(GetCurrentProcess(), 3);
        }
        beginHandling();

        char message[256] = "unknown";
        try {
            const std::exception_ptr current = std::current_exception();
            if (current) {
                std::rethrow_exception(current);
            }
        } catch (const std::exception& error) {
            const char* text = error.what();
            if (text != nullptr) {
                std::strncpy(message, text, sizeof(message) - 1);
                message[sizeof(message) - 1] = '\0';
            }
        } catch (...) {
            std::strncpy(message, "unknown C++ exception", sizeof(message) - 1);
            message[sizeof(message) - 1] = '\0';
        }

        buildReport("unhandled C++ exception", nullptr, true);
        appendFormat("Exception      : %s\n", message);
        writeReportFile();

        std::abort();
    }

} // namespace

void install() {
    static bool installed = false;
    if (installed) {
        return;
    }
    installed = true;

    SetUnhandledExceptionFilter(unhandledExceptionFilter);
    AddVectoredExceptionHandler(1, vectoredExceptionHandler);
    std::set_terminate(terminateHandler);
    std::signal(SIGABRT, signalAbortHandler);
}

void setLogSource(const Logger* logger) {
    g_logger = logger;
}

const char* parseTestCrashMode(int argc, char** argv) {
    static constexpr const char* prefix = "--crash-test=";
    const size_t prefixLength = std::strlen(prefix);

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }
        if (std::strncmp(argv[i], prefix, prefixLength) == 0) {
            return argv[i] + prefixLength;
        }
    }
    return nullptr;
}

void triggerTestCrash(const char* mode) {
    if (mode != nullptr && std::strcmp(mode, "throw") == 0) {
        throw std::runtime_error("crash-test: unhandled C++ exception");
    }
    if (mode != nullptr && std::strcmp(mode, "abort") == 0) {
        std::abort();
    }
    if (mode != nullptr && std::strcmp(mode, "stack") == 0) {
        // alloca in a loop grows the frame until the guard page is hit.
        for (;;) {
            volatile char* block = static_cast<volatile char*>(alloca(64 * 1024));
            block[0] = 1;
        }
    }

    volatile int* pointer = nullptr;
    *pointer = 1;
    std::abort();
}

} // namespace app::crash
