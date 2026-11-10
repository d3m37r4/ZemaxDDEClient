#include <string_view>

#include <gtest/gtest.h>

#include "gui/shortcuts.h"

namespace {

using gui::shortcuts::kEntries;
using gui::shortcuts::kEntryCount;
using gui::shortcuts::kHintRowCount;
using gui::shortcuts::kHintRows;
using gui::shortcuts::buildDisplaySections;

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
        EXPECT_FALSE(std::string_view(kEntries[i].section).empty()) << "Empty section at index " << i;
        EXPECT_FALSE(std::string_view(kEntries[i].hint).empty()) << "Empty hint at index " << i;
        EXPECT_FALSE(std::string_view(kEntries[i].action).empty()) << "Empty action at index " << i;
    }
}

TEST(ShortcutsDisplay, SectionsAreUniqueAndNonEmpty) {
    const auto sections = buildDisplaySections();
    ASSERT_FALSE(sections.empty());
    EXPECT_EQ(std::string_view(sections.front().name), "File");
    for (size_t i = 0; i < sections.size(); ++i) {
        EXPECT_FALSE(std::string_view(sections[i].name).empty());
        EXPECT_FALSE(sections[i].rows.empty()) << sections[i].name;
        for (size_t j = i + 1; j < sections.size(); ++j) {
            EXPECT_NE(std::string_view(sections[i].name), std::string_view(sections[j].name));
        }
    }
}

TEST(ShortcutsDisplay, RepeatedActionsAreMerged) {
    const auto sections = buildDisplaySections();
    int hits = 0;
    for (const auto& section : sections) {
        for (const auto& row : section.rows) {
            if (std::string_view(row.action) == "Keyboard shortcuts help") {
                ++hits;
                EXPECT_EQ(row.hint, "F1 / Ctrl+/");
            }
        }
    }
    EXPECT_EQ(hits, 1);
}

TEST(ShortcutsDisplay, LogsSectionEndsWithMouseHints) {
    const auto sections = buildDisplaySections();
    ASSERT_FALSE(sections.empty());
    const auto& logs = sections.back();
    EXPECT_EQ(std::string_view(logs.name), "Logs");
    ASSERT_GE(logs.rows.size(), static_cast<size_t>(kHintRowCount));
    for (int h = 0; h < kHintRowCount; ++h) {
        const auto& row = logs.rows[logs.rows.size() - static_cast<size_t>(kHintRowCount) + h];
        EXPECT_EQ(std::string_view(row.action), std::string_view(kHintRows[h].action));
        EXPECT_EQ(row.hint, std::string(kHintRows[h].hint));
    }
}

TEST(ShortcutsTable, MouseHintsAreWellFormed) {
    for (int i = 0; i < kHintRowCount; ++i) {
        EXPECT_FALSE(std::string_view(kHintRows[i].section).empty());
        EXPECT_FALSE(std::string_view(kHintRows[i].action).empty());
        EXPECT_FALSE(std::string_view(kHintRows[i].hint).empty());
        for (int j = i + 1; j < kHintRowCount; ++j) {
            const bool same = std::string_view(kHintRows[i].section) == kHintRows[j].section &&
                              std::string_view(kHintRows[i].action) == kHintRows[j].action &&
                              std::string_view(kHintRows[i].hint) == kHintRows[j].hint;
            EXPECT_FALSE(same) << "Duplicate hint row at " << i << " and " << j;
        }
    }
}

TEST(ShortcutsTable, SectionsAreContiguous) {
    std::string_view last;
    bool seen[6] = {};
    const std::string_view known[] = {"File", "View", "Info", "DDE", "Dialogs", "Logs"};
    for (int i = 0; i < kEntryCount; ++i) {
        const std::string_view section(kEntries[i].section);
        int knownIdx = -1;
        for (int k = 0; k < 6; ++k) {
            if (section == known[k]) {
                knownIdx = k;
                break;
            }
        }
        EXPECT_GE(knownIdx, 0) << "Unknown section at index " << i;
        if (section != last) {
            EXPECT_FALSE(seen[knownIdx]) << "Section reappears: " << section;
            seen[knownIdx] = true;
            last = section;
        }
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
