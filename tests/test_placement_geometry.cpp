#include <gtest/gtest.h>

#include "gui/placement_geometry.h"

using namespace gui::placement;

TEST(PlacementGeometry, HalvesSplitTheWorkAreaWithoutGaps) {
    // Odd width: the right half must take the extra pixel, or the two halves
    // leave a seam that no window covers.
    const Rect work{0, 0, 1921, 1080};
    const Rect left = rectFor(work, Target::LeftHalf);
    const Rect right = rectFor(work, Target::RightHalf);
    EXPECT_EQ(left.x + left.w, right.x);
    EXPECT_EQ(right.x + right.w, work.x + work.w);
    EXPECT_EQ(left.w + right.w, work.w);
}

TEST(PlacementGeometry, QuartersFillTheHalf) {
    const Rect work{100, 50, 1000, 700};
    const Rect topLeft = rectFor(work, Target::TopLeftQuarter);
    const Rect topRight = rectFor(work, Target::TopRightQuarter);
    const Rect bottomLeft = rectFor(work, Target::BottomLeftQuarter);
    EXPECT_EQ(topLeft.x, work.x);
    EXPECT_EQ(topLeft.y, work.y);
    EXPECT_EQ(topLeft.w, topRight.x - work.x);
    EXPECT_EQ(topLeft.h, bottomLeft.y - work.y);
    EXPECT_EQ(topLeft.h + bottomLeft.h, work.h);
}

TEST(PlacementGeometry, MaximizeCoversTheWholeWorkArea) {
    const Rect work{1920, 0, 1920, 1040};
    EXPECT_EQ(rectFor(work, Target::Maximize), work);
}

TEST(PlacementGeometry, EdgeZonesUseTheWorkAreaOffset) {
    // Second monitor on the right; the taskbar takes 40 px at the bottom.
    const Rect work{1920, 0, 1920, 1040};
    EXPECT_EQ(edgeZoneAt(work, 1921, 500, 8), Target::LeftHalf);
    EXPECT_EQ(edgeZoneAt(work, 3839, 500, 8), Target::RightHalf);
    EXPECT_EQ(edgeZoneAt(work, 2800, 2, 8), Target::Maximize);
    EXPECT_EQ(edgeZoneAt(work, 1921, 2, 8), Target::TopLeftQuarter);
    EXPECT_EQ(edgeZoneAt(work, 3839, 1038, 8), Target::BottomRightQuarter);
}

TEST(PlacementGeometry, NothingSnapsAwayFromTheEdges) {
    const Rect work{0, 0, 1920, 1040};
    EXPECT_EQ(edgeZoneAt(work, 960, 520, 8), std::nullopt);
    // Outside the work area: another monitor or beyond the screen.
    EXPECT_EQ(edgeZoneAt(work, -1, 520, 8), std::nullopt);
    EXPECT_EQ(edgeZoneAt(work, 1920, 520, 8), std::nullopt);
    // The taskbar strip below the work area is not a snap zone either.
    EXPECT_EQ(edgeZoneAt(work, 960, 1040, 8), std::nullopt);
}
