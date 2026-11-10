#include <gtest/gtest.h>

#include <string_view>

#include "dde/utils.h"

namespace ZemaxDDE {
namespace {

TEST(UpdateStatus, ParsesUpdated) {
    EXPECT_EQ(parseUpdateStatus("0"), UpdateStatus::Updated);
    EXPECT_EQ(parseUpdateStatus(" 0 \r\n"), UpdateStatus::Updated);
    EXPECT_EQ(parseUpdateStatus(std::string_view("0\0", 2)), UpdateStatus::Updated);
}

TEST(UpdateStatus, ParsesNotUpdated) {
    EXPECT_EQ(parseUpdateStatus("-1"), UpdateStatus::NotUpdated);
    EXPECT_EQ(parseUpdateStatus(" -1\r\n"), UpdateStatus::NotUpdated);
    EXPECT_EQ(parseUpdateStatus(std::string_view("-1\0", 3)), UpdateStatus::NotUpdated);
}

TEST(UpdateStatus, ParsesUnknown) {
    EXPECT_EQ(parseUpdateStatus(""), UpdateStatus::Unknown);
    EXPECT_EQ(parseUpdateStatus("   "), UpdateStatus::Unknown);
    EXPECT_EQ(parseUpdateStatus("0,1"), UpdateStatus::Unknown);
    EXPECT_EQ(parseUpdateStatus("error"), UpdateStatus::Unknown);
}

} // namespace
} // namespace ZemaxDDE
