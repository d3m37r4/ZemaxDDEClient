#include <gtest/gtest.h>

#include <filesystem>

#include "gui/font_catalog.h"

TEST(FontCatalog, DefaultFontHasCyrillic) {
    const std::string path = gui::fonts::defaultFontPath();
    ASSERT_FALSE(path.empty());
    ASSERT_TRUE(std::filesystem::exists(path)) << path;
    EXPECT_TRUE(gui::fonts::hasCyrillic(path));
}

TEST(FontCatalog, SymbolFontIsRejected) {
    // Wingdings maps only the private use area and has no Cyrillic at all: it is
    // the font that crashed the atlas build, so the check must reject it before
    // ImGui sees the file.
    const std::filesystem::path wingding =
        std::filesystem::path(gui::fonts::defaultFontPath()).parent_path() / L"wingding.ttf";
    if (!std::filesystem::exists(wingding)) {
        GTEST_SKIP() << "wingding.ttf is not installed";
    }
    EXPECT_FALSE(gui::fonts::hasCyrillic(wingding.string()));
}

TEST(FontCatalog, MissingFileIsRejected) {
    EXPECT_FALSE(gui::fonts::hasCyrillic("C:\\Windows\\Fonts\\definitely-not-a-font.ttf"));
}

TEST(FontCatalog, SystemFontsAreListed) {
    const auto fonts = gui::fonts::listSystemFonts();
    ASSERT_FALSE(fonts.empty());
    for (const auto& font : fonts) {
        EXPECT_FALSE(font.path.empty());
        EXPECT_FALSE(font.name.empty());
    }
}
