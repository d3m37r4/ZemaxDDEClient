#pragma once

#include <string>
#include <string_view>

/// Serialized layout preset: the ImGui ini (docking/positions), the panel
/// visibility map and the chrome state (minimized tiles + floating rects).
/// ImGui-free so the file format is unit-testable.
namespace gui::layout {

    struct Snapshot {
        int imguiVersion = 0;
        std::string imguiIni;
        std::string windowsJson;
        std::string chromeJson;
    };

    std::string serializeSnapshot(const Snapshot& snapshot);

    /// Returns false for malformed data or a missing version field.
    bool deserializeSnapshot(std::string_view text, Snapshot& out);

}
