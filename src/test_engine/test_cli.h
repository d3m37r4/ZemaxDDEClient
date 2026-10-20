#pragma once

// Command-line options for the Dear ImGui Test Engine integration.
//
// The test engine is compiled in only when the CMake option
// ZEMAX_ENABLE_TEST_ENGINE is ON (see CMakeLists.txt). In that case the
// application accepts the options below:
//
//   --test-engine            Show the interactive Test Engine UI (developer tool)
//   --run-tests <filter>     Run the matching tests, then exit with a status code
//   --test-results <file>    Export test results as JUnit XML to <file>
//   --test-verbose           Raise the log verbosity of the test run
//
// <filter> uses the Test Engine query syntax, e.g. "all", "tests" or a
// pattern such as "^smoke_". See ImGuiTestEngine_QueueTests().

#include <string>

namespace app::testing {

struct TestCliOptions {
    // Show the interactive Test Engine UI inside the application window.
    bool showEngineUi = false;
    // Run the matching tests and quit when the queue drains.
    bool runTests = false;
    // Query passed to ImGuiTestEngine_QueueTests(); "all" when runTests is set.
    std::string filter = "all";
    // Optional JUnit XML export path.
    std::string resultsFile;
    // Raise verbosity (useful when a test fails and needs investigation).
    bool verbose = false;

    /// True when the process was asked to do anything test related.
    [[nodiscard]] bool isTestMode() const noexcept { return showEngineUi || runTests; }
};

/// Parse the command line. Unknown options are ignored so that the regular
/// application start-up path stays unaffected.
TestCliOptions parseTestCliOptions(int argc, char** argv);

} // namespace app::testing
