#include <gtest/gtest.h>

#include "app/utils.h"

namespace app {
namespace {

TEST(ClampWithFeedback, BelowMinClampsAndReports) {
    int v = 5;
    EXPECT_TRUE(ClampWithFeedback(v, 33, 16385));
    EXPECT_EQ(v, 33);
}

TEST(ClampWithFeedback, AboveMaxClampsAndReports) {
    int v = 99999;
    EXPECT_TRUE(ClampWithFeedback(v, 33, 16385));
    EXPECT_EQ(v, 16385);
}

TEST(ClampWithFeedback, InRangeKeepsValueSilent) {
    int v = 65;
    EXPECT_FALSE(ClampWithFeedback(v, 33, 16385));
    EXPECT_EQ(v, 65);
}

TEST(ClampWithFeedback, AtBoundsKeepsValueSilent) {
    int lo = 33;
    EXPECT_FALSE(ClampWithFeedback(lo, 33, 16385));
    EXPECT_EQ(lo, 33);
    int hi = 16385;
    EXPECT_FALSE(ClampWithFeedback(hi, 33, 16385));
    EXPECT_EQ(hi, 16385);
}

TEST(ClampWithFeedback, WorksForDoubles) {
    double v = 500.0;
    EXPECT_TRUE(ClampWithFeedback(v, -360.0, 360.0));
    EXPECT_DOUBLE_EQ(v, 360.0);
    double ok = 45.0;
    EXPECT_FALSE(ClampWithFeedback(ok, -360.0, 360.0));
    EXPECT_DOUBLE_EQ(ok, 45.0);
}

TEST(SnapOddWithFeedback, EvenSnapsUpAndReports) {
    int v = 100;
    EXPECT_TRUE(SnapOddWithFeedback(v));
    EXPECT_EQ(v, 101);
}

TEST(SnapOddWithFeedback, OddKeepsValueSilent) {
    int v = 65;
    EXPECT_FALSE(SnapOddWithFeedback(v));
    EXPECT_EQ(v, 65);
}

} // namespace
} // namespace app
