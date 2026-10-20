#include "test_engine/test_engine.h"
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

    if (!options.resultsFile.empty()) {
        test_io.ExportResultsFilename = options.resultsFile.c_str();
        test_io.ExportResultsFormat = ImGuiTestEngineExportFormat_JUnitXml;
    }

    // Bind to the running ImGui context. This registers the engine hooks
    // (NewFramePre/Post, EndFramePre, RenderPre/Post) with the context.
    ImGuiTestEngine_Start(m_engine, ImGui::GetCurrentContext());
    ImGuiTestEngine_InstallDefaultCrashHandler();

    if (options.runTests) {
        ImGuiTestEngine_QueueTests(m_engine, ImGuiTestGroup_Unknown, options.filter.c_str(),
                                   ImGuiTestRunFlags_RunFromCommandLine);
        m_runTests = true;
        std::fprintf(stdout, "[TEST] Queued tests matching '%s'\n", options.filter.c_str());
    }
}

void TestEngine::registerTests(app::testing::TestAppContext& app) {
    if (!m_engine) {
        return;
    }
    app::testing::registerApplicationTests(m_engine, &app);
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
    if (!m_engine) {
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
