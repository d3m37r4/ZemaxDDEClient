// Aggregates all application test suites.
//
// Each suite lives in its own file under tests/ and registers itself here.

#include "test_engine/test_engine.h"

#include "imgui_test_engine/imgui_te_engine.h"

namespace app::testing {

void registerSmokeTests(ImGuiTestEngine* engine, TestAppContext* app);
void registerPreferencesTests(ImGuiTestEngine* engine, TestAppContext* app);
void registerDialogTests(ImGuiTestEngine* engine, TestAppContext* app);
void registerLogsDdeTests(ImGuiTestEngine* engine, TestAppContext* app);
void registerThemeTests(ImGuiTestEngine* engine, TestAppContext* app);
void registerCaptureTests(ImGuiTestEngine* engine, TestAppContext* app);

void registerApplicationTests(ImGuiTestEngine* engine, TestAppContext* app) {
    registerSmokeTests(engine, app);
    registerPreferencesTests(engine, app);
    registerDialogTests(engine, app);
    registerLogsDdeTests(engine, app);
    registerThemeTests(engine, app);
    registerCaptureTests(engine, app);
}

} // namespace app::testing
