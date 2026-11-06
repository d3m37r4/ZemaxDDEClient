#include <gtest/gtest.h>

#include <vector>

#include "gui/plot_image_export_util.h"
#include "app/models/surface_data.h"

using gui::plot_export::ImageFormat;

namespace {

app::models::SurfaceData makeSurface(int id, double angle, int slot) {
    app::models::SurfaceData surface;
    surface.id = id;
    surface.angle = angle;
    surface.sourceSlot = slot;
    return surface;
}

}  // namespace

TEST(PlotImageExport, DefaultNameTrimsAngleZeros) {
    EXPECT_EQ(gui::plot_export::defaultImageName("sag", makeSurface(3, 0.0, 0)),
              "sag_surf3_0deg_slot0");
    EXPECT_EQ(gui::plot_export::defaultImageName("dev", makeSurface(3, 22.5, 1)),
              "dev_surf3_22.5deg_slot1");
    EXPECT_EQ(gui::plot_export::defaultImageName("map", makeSurface(7, 100.0, -1)),
              "map_surf7_100deg");
}

TEST(PlotImageExport, DefaultNameDropsUnknownSlot) {
    EXPECT_EQ(gui::plot_export::defaultImageName("compare", makeSurface(1, 0.0, -1)),
              "compare_surf1_0deg");
}

TEST(PlotImageExport, FormatFollowsExtension) {
    EXPECT_EQ(gui::plot_export::imageFormatFromPath("a.png"), ImageFormat::Png);
    EXPECT_EQ(gui::plot_export::imageFormatFromPath("a.PNG"), ImageFormat::Png);
    EXPECT_EQ(gui::plot_export::imageFormatFromPath("a.jpg"), ImageFormat::Jpeg);
    EXPECT_EQ(gui::plot_export::imageFormatFromPath("a.JPeG"), ImageFormat::Jpeg);
    EXPECT_EQ(gui::plot_export::imageFormatFromPath("a.bmp"), ImageFormat::Bmp);
    EXPECT_EQ(gui::plot_export::imageFormatFromPath("C:\\dir\\shot.png"), ImageFormat::Png);
}

TEST(PlotImageExport, FormatRejectsUnknownPaths) {
    EXPECT_EQ(gui::plot_export::imageFormatFromPath("shot"), ImageFormat::Unknown);
    EXPECT_EQ(gui::plot_export::imageFormatFromPath("dir.v2/shot"), ImageFormat::Unknown);
    EXPECT_EQ(gui::plot_export::imageFormatFromPath("shot.jpg/jpeg"), ImageFormat::Unknown);
    EXPECT_EQ(gui::plot_export::imageFormatFromPath("shot.txt"), ImageFormat::Unknown);
}

TEST(PlotImageExport, FlipRowsSwapsTopAndBottom) {
    // Two RGBA rows: [1..4] on top, [5..8] below.
    std::vector<unsigned char> pixels = {1, 2, 3, 4, 5, 6, 7, 8};
    gui::plot_export::flipRows(pixels, 1, 2);
    EXPECT_EQ(pixels, (std::vector<unsigned char>{5, 6, 7, 8, 1, 2, 3, 4}));
}

TEST(PlotImageExport, FlipRowsKeepsMiddleRowOfOddHeight) {
    std::vector<unsigned char> pixels = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
    };
    gui::plot_export::flipRows(pixels, 1, 3);
    EXPECT_EQ(pixels, (std::vector<unsigned char>{9, 10, 11, 12, 5, 6, 7, 8, 1, 2, 3, 4}));
}

TEST(PlotImageExport, FlipRowsIgnoresDegenerateInput) {
    std::vector<unsigned char> pixels = {1, 2, 3, 4};
    gui::plot_export::flipRows(pixels, 1, 0);
    EXPECT_EQ(pixels, (std::vector<unsigned char>{1, 2, 3, 4}));

    std::vector<unsigned char> tooSmall = {1, 2, 3, 4};
    gui::plot_export::flipRows(tooSmall, 1, 4);
    EXPECT_EQ(tooSmall, (std::vector<unsigned char>{1, 2, 3, 4}));
}
