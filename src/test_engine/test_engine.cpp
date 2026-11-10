#include "test_engine/test_engine.h"
#include "test_engine/test_capture.h"
#include "test_engine/test_cli.h"

#include "imgui_test_engine/imgui_te_engine.h"
#include "imgui_test_engine/imgui_te_exporters.h"
#include "imgui_test_engine/imgui_te_ui.h"

#include <cstdio>

TestEngine::TestEngine() = default;

TestEngine::~TestEngine() {
    // Defensive: the application calls stop()/destroy() explicitly in the
    // correct order. If it did not, make sure we do not leak the engine.
    if (m_engine) {
        ImGuiTestEngine_Stop(m_engine);
        ImGuiTestEngine_DestroyContext(m_engine);
        m_engine = nullptr;
    }
}

void TestEngine::start(const app::testing::TestCliOptions& options) {
    m_engine = ImGuiTestEngine_CreateContext();
    if (!m_engine) {
        std::fprintf(stderr, "[TEST] Failed to create the test engine context\n");
        return;
    }

    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(m_engine);
    test_io.ConfigVerboseLevel = options.verbose ? ImGuiTestVerboseLevel_Debug : ImGuiTestVerboseLevel_Info;
    test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    test_io.ConfigRunSpeed = ImGuiTestRunSpeed_Fast;
    test_io.ConfigStopOnError = false;
    // Mirror the log to the console so a failing test can be diagnosed
    // without opening the Test Engine UI.
    test_io.ConfigLogToTTY = true;
    // Screen capture (screenshots, videos) needs a callback that reads the
    // rendered frame back from the OpenGL framebuffer.
    test_io.ScreenCaptureFunc = &app::testing::captureScreenRegion;

    m_runTests = options.runTests;
    m_testFilter = options.filter;
    m_showEngineUi = options.showEngineUi;

    if (!options.resultsFile.empty()) {
        test_io.ExportResultsFilename = options.resultsFile.c_str();
        test_io.ExportResultsFormat = ImGuiTestEngineExportFormat_JUnitXml;
    }

    // Bind to the running ImGui context. This registers the engine hooks
    // (NewFramePre/Post, EndFramePre, RenderPre/Post) with the context.
    ImGuiTestEngine_Start(m_engine, ImGui::GetCurrentContext());
    ImGuiTestEngine_InstallDefaultCrashHandler();
}

void TestEngine::registerTests(app::testing::TestAppContext& app) {
    if (!m_engine) {
        return;
    }
    app::testing::registerApplicationTests(m_engine, &app);

    // Queue the tests only after they have been registered, otherwise the
    // queue would be empty when ImGuiTestEngine_QueueTests() runs.
    if (m_runTests) {
        ImGuiTestEngine_QueueTests(m_engine, ImGuiTestGroup_Unknown, m_testFilter.c_str(),
                                   ImGuiTestRunFlags_RunFromCommandLine);
        std::fprintf(stdout, "[TEST] Queued tests matching '%s'\n", m_testFilter.c_str());
    }
}

void TestEngine::stop() {
    if (!m_engine) {
        return;
    }
    ImGuiTestEngine_Stop(m_engine);
}

void TestEngine::destroy() {
    if (!m_engine) {
        return;
    }
    ImGuiTestEngine_DestroyContext(m_engine);
    m_engine = nullptr;
}

void TestEngine::renderUi() {
    if (!m_engine || !m_showEngineUi) {
        return;
    }
    ImGuiTestEngine_ShowTestEngineWindows(m_engine, nullptr);
}

void TestEngine::preSwap() {
    if (!m_engine) {
        return;
    }
    ImGuiTestEngine_PreSwap(m_engine);
}

void TestEngine::postSwap() {
    if (!m_engine) {
        return;
    }
    ImGuiTestEngine_PostSwap(m_engine);
}

bool TestEngine::isTestRunFinished() const {
    if (!m_engine || !m_runTests) {
        return false;
    }
    // The queue is emptied by the engine once the last queued test completed.
    return ImGuiTestEngine_IsTestQueueEmpty(m_engine);
}

bool TestEngine::isRunningTests() const {
    if (!m_engine) {
        return false;
    }
    return ImGuiTestEngine_GetIO(m_engine).IsRunningTests;
}

void TestEngine::printResultSummary() const {
    if (!m_engine) {
        return;
    }
    ImGuiTestEngine_PrintResultSummary(m_engine);
}

int TestEngine::exitCode() const {
    if (!m_engine || !m_runTests) {
        return 0;
    }
    ImGuiTestEngineResultSummary summary;
    ImGuiTestEngine_GetResultSummary(m_engine, &summary);
    return (summary.CountSuccess == summary.CountTested && summary.CountTested > 0) ? 0 : 1;
}
