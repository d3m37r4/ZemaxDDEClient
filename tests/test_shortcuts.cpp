#include <string_view>

#include <gtest/gtest.h>

#include "gui/shortcuts.h"

namespace {

using gui::shortcuts::kEntries;
using gui::shortcuts::kEntryCount;

TEST(ShortcutsTable, ChordsAreUnique) {
    for (int i = 0; i < kEntryCount; ++i) {
        for (int j = i + 1; j < kEntryCount; ++j) {
            const bool sameChord = (kEntries[i].key == kEntries[j].key) && (kEntries[i].mods == kEntries[j].mods);
            EXPECT_FALSE(sameChord) << "Duplicate chord: " << kEntries[i].hint << " (" << kEntries[i].id << " vs "
                                    << kEntries[j].id << ")";
        }
    }
}

TEST(ShortcutsTable, IdsHintsAndActionsAreNonEmpty) {
    for (int i = 0; i < kEntryCount; ++i) {
        EXPECT_FALSE(std::string_view(kEntries[i].id).empty()) << "Empty id at index " << i;
        EXPECT_FALSE(std::string_view(kEntries[i].hint).empty()) << "Empty hint at index " << i;
        EXPECT_FALSE(std::string_view(kEntries[i].action).empty()) << "Empty action at index " << i;
    }
}

TEST(ShortcutsTable, HelpHasTwoChords) {
    int helpCount = 0;
    for (int i = 0; i < kEntryCount; ++i) {
        if (std::string_view(kEntries[i].action) == "Keyboard shortcuts help") {
            ++helpCount;
        }
    }
    EXPECT_EQ(helpCount, 2) << "Help must be reachable via F1 and Ctrl+/";
}

TEST(ShortcutsTable, DdeConnectDisconnectAreSplitWithShift) {
    using gui::shortcuts::Mod_Ctrl;
    using gui::shortcuts::Mod_Shift;
    bool connectFound = false;
    bool disconnectFound = false;
    for (int i = 0; i < kEntryCount; ++i) {
        const std::string_view id(kEntries[i].id);
        if (id == "dde-connect") {
            EXPECT_EQ(kEntries[i].mods, Mod_Ctrl | Mod_Shift);
            connectFound = true;
        }
        if (id == "dde-disconnect") {
            EXPECT_EQ(kEntries[i].mods, Mod_Ctrl | Mod_Shift);
            disconnectFound = true;
        }
        EXPECT_NE(id, "dde-toggle") << "Toggle must be replaced by split connect/disconnect";
    }
    EXPECT_TRUE(connectFound);
    EXPECT_TRUE(disconnectFound);
}

} // namespace
