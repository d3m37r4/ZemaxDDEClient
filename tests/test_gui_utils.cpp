#include <gtest/gtest.h>

#include "gui/utils.h"

namespace gui {
namespace {

TEST(FormatCount, SmallNumbersStayPlain) {
    EXPECT_EQ(formatCount(0), "0");
    EXPECT_EQ(formatCount(7), "7");
    EXPECT_EQ(formatCount(999), "999");
}

TEST(FormatCount, GroupsThousands) {
    EXPECT_EQ(formatCount(1000), "1 000");
    EXPECT_EQ(formatCount(12060), "12 060");
    EXPECT_EQ(formatCount(12060000), "12 060 000");
}

TEST(FormatCount, KeepsTheSignOutside) {
    EXPECT_EQ(formatCount(-12060), "-12 060");
}

} // namespace
} // namespace gui
