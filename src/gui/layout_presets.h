#pragma once

#include <string>

#include "gui/layout_snapshot.h"

class DockableWindowsManager;
class Logger;

/// Default-layout presets: capture the current layout (ImGui ini, panel
/// visibility, chrome state) and restore it either on demand or at startup.
namespace gui::layout {

    Snapshot captureCurrentLayout(const DockableWindowsManager& windows);

    /// Applies the snapshot to the running session. Returns false when the
    /// snapshot was written by another Dear ImGui build (layout formats are
    /// not forward compatible).
    bool applySnapshot(const Snapshot& snapshot, DockableWindowsManager& windows);

    bool saveDefaultLayout(const std::string& path, const DockableWindowsManager& windows);
    bool loadDefaultLayout(const std::string& path, Snapshot& out);

    /// Empty ini (default docking), registry visibility and clean chrome state.
    void resetToFactoryLayout(DockableWindowsManager& windows);

    /// Startup helper: applies the saved default when it exists and matches the
    /// running ImGui version. Never throws; failures are logged.
    bool applySavedLayoutOnStartup(const std::string& path, DockableWindowsManager& windows,
                                   Logger& logger);

}
