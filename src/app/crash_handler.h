#pragma once

class Logger;

namespace app::crash {
    /// Installs the crash handlers: the unhandled exception filter, the
    /// vectored exception handler and the terminate handler. Call it as the
    /// first statement of main(), before anything can crash. Later calls are
    /// no-ops.
    void install();

    /// Gives the crash report access to the recent log lines. Optional: the
    /// report works without it, it just misses the last log lines.
    void setLogSource(const Logger* logger);

    /// Returns the crash-test mode from the command line ("null", "stack",
    /// "throw", "abort") or nullptr when the flag is absent.
    const char* parseTestCrashMode(int argc, char** argv);

    /// Crashes the process on purpose to verify the crash pipeline. Never
    /// returns.
    [[noreturn]] void triggerTestCrash(const char* mode);
}
