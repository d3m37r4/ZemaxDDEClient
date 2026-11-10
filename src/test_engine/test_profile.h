#pragma once

// Isolated profile for test runs.
//
// The application persists its state in settings.json, imgui.ini and
// windows.json. A test run mutates all of them (opening panels, switching the
// theme, editing preferences), so it must not touch the user's real profile.
//
// This module redirects those files into a throw-away directory for the
// duration of the test run. Nothing here affects regular application start-up.

#include <string>

namespace app::testing {

/// Redirect settings.json / imgui.ini / windows.json into a temporary
/// directory. Call before the application reads any persisted state.
/// Returns false when the directory could not be created (in that case the
/// application keeps using the regular paths).
bool useIsolatedTestProfile();

/// Absolute path of the isolated profile directory (empty when not active).
[[nodiscard]] const std::string& isolatedProfilePath();

/// Remove the isolated profile directory and everything in it.
void cleanupIsolatedTestProfile();

} // namespace app::testing
