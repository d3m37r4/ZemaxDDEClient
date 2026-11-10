#pragma once

// Integration of the Dear ImGui Test Engine into the application.
//
// The test engine drives the application the way a user would: it injects
// mouse and keyboard input into ImGui's IO and then queries the resulting
// widget state. This makes it possible to assert on real UI behaviour
// (menus, dock panels, dialogs) instead of on internal state only.
//
// This header is only compiled when the CMake option ZEMAX_ENABLE_TEST_ENGINE
// is ON, so the shipped release binary contains none of this code.

#include <string>

namespace gui {
    class GuiManager;
}

class DockableWindowsManager;

namespace app::testing {

struct TestCliOptions;

/// Application state a test may inspect. Passed to every test through
/// ImGuiTest::UserData.
struct TestAppContext {
    gui::GuiManager* gui = nullptr;
    DockableWindowsManager* windows = nullptr;
};

} // namespace app::testing

// Forward declaration of the engine type (avoids pulling the engine headers
// into every translation unit).
struct ImGuiTestEngine;

/// Owns the Dear ImGui Test Engine context and drives its lifecycle.
///
/// Lifecycle constraints imposed by the upstream engine:
///   1. ImGuiTestEngine_Start() must run after ImGui::CreateContext().
///   2. ImGuiTestEngine_Stop() must run before ImGui::DestroyContext().
///   3. ImGuiTestEngine_DestroyContext() must run after ImGui::DestroyContext()
///      so that the engine's own .ini data gets flushed to disk.
class TestEngine {
public:
    TestEngine();
    ~TestEngine();

    TestEngine(const TestEngine&) = delete;
    TestEngine& operator=(const TestEngine&) = delete;

    /// Create the engine context and start it against the current ImGui
    /// context. Must be called after ImGui::CreateContext().
    void start(const app::testing::TestCliOptions& options);

    /// Register the application test suites. Called from start().
    void registerTests(app::testing::TestAppContext& app);

    /// Stop the engine coroutine. Must run before ImGui::DestroyContext().
    void stop();

    /// Destroy the engine context. Must run after ImGui::DestroyContext().
    void destroy();

    /// Render the interactive Test Engine UI. Call once per frame, inside the
    /// ImGui frame (between NewFrame and Render).
    void renderUi();

    /// Called right before the framebuffer swap.
    void preSwap();

    /// Called right after the framebuffer swap (needed for screen capture).
    void postSwap();

    /// True when the engine was asked to run tests and the queue has drained.
    /// The application uses this to quit on its own after a test run.
    [[nodiscard]] bool isTestRunFinished() const;

    /// True while the engine is executing a test.
    [[nodiscard]] bool isRunningTests() const;

    /// Print a summary of the last run to the console.
    void printResultSummary() const;

    /// Process exit code: 0 when every test passed, 1 otherwise.
    [[nodiscard]] int exitCode() const;

    [[nodiscard]] ImGuiTestEngine* engine() const { return m_engine; }

private:
    ImGuiTestEngine* m_engine = nullptr;
    bool m_runTests = false;
    bool m_showEngineUi = false;
    std::string m_testFilter = "all";
};

namespace app::testing {

/// Register all application test suites with the given engine.
void registerApplicationTests(ImGuiTestEngine* engine, TestAppContext* app);
/// Names of the test suites, used for grouping in the Test Engine UI.
namespace test_categories {
    inline constexpr const char* kSmoke = "smoke";
    inline constexpr const char* kPreferences = "preferences";
    inline constexpr const char* kDialogs = "dialogs";
    inline constexpr const char* kLogsDde = "logs_dde";
    inline constexpr const char* kTheme = "theme";
    inline constexpr const char* kCapture = "capture";
} // namespace test_categories

} // namespace app::testing
