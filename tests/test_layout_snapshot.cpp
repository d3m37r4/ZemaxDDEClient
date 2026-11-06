#include <gtest/gtest.h>

#include "gui/layout_snapshot.h"

using gui::layout::Snapshot;

TEST(LayoutSnapshot, RoundTrip) {
    Snapshot original;
    original.imguiVersion = 19300;
    original.imguiIni = "[Window][Logs]\nPos=0,0\n";
    original.windowsJson = "{\"Logs\": true}";
    original.chromeJson = "{\"plot:x\": {}}";

    const std::string text = gui::layout::serializeSnapshot(original);
    Snapshot parsed;
    ASSERT_TRUE(gui::layout::deserializeSnapshot(text, parsed));
    EXPECT_EQ(parsed.imguiVersion, original.imguiVersion);
    EXPECT_EQ(parsed.imguiIni, original.imguiIni);
    EXPECT_EQ(parsed.windowsJson, original.windowsJson);
    EXPECT_EQ(parsed.chromeJson, original.chromeJson);
}

TEST(LayoutSnapshot, RejectsMalformedData) {
    Snapshot parsed;
    EXPECT_FALSE(gui::layout::deserializeSnapshot("", parsed));
    EXPECT_FALSE(gui::layout::deserializeSnapshot("not json", parsed));
    EXPECT_FALSE(gui::layout::deserializeSnapshot("[1,2,3]", parsed));
    EXPECT_FALSE(gui::layout::deserializeSnapshot("{}", parsed));
}

TEST(LayoutSnapshot, RejectsMissingVersion) {
    Snapshot parsed;
    EXPECT_FALSE(gui::layout::deserializeSnapshot(
        R"({"imguiIni":"x","windowsJson":"{}","chromeJson":"{}"})", parsed));
    EXPECT_TRUE(gui::layout::deserializeSnapshot(
        R"({"imguiVersion":19300,"imguiIni":"x","windowsJson":"{}","chromeJson":"{}"})", parsed));
}
