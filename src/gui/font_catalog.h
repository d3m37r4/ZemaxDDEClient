#pragma once

// System font discovery for the interface font picker. Only the file list is
// produced here: whether a font can actually be drawn is decided when it is
// loaded into the ImGui atlas.

#include <string>
#include <vector>

namespace gui::fonts {
    struct FontEntry {
        /// Full path, the form ImGui loads and the settings store.
        std::string path;
        /// File stem ("segoeui", "consola", ...): what the search box matches.
        std::string name;
    };

    /// Font used when the user keeps the default choice.
    std::string defaultFontPath();

    /// True when the file contains the Cyrillic glyphs the UI needs. The cmap
    /// is read here, before ImGui bakes anything: a font with no glyph in the
    /// requested ranges crashes the atlas build, so it must never reach it.
    bool hasCyrillic(const std::string& path);

    /// Every .ttf/.otf/.ttc in the Windows font folder, sorted by name.
    std::vector<FontEntry> listSystemFonts();
} // namespace gui::fonts
