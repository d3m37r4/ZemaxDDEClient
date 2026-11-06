#include <gtest/gtest.h>

#include <chrono>

#include "gui/log_time_filter.h"

using namespace std::chrono;
using gui::logs::inTimeRange;
using gui::logs::parseLogTimestamp;

TEST(LogTimeFilter, ParsesLoggerFormat) {
    const auto timestamp = parseLogTimestamp("[04.11.2026 - 21:32:18] [DDE] Sent request #73");
    ASSERT_TRUE(timestamp.has_value());
    const auto expected = sys_days{year{2026} / November / day{4}} + hours{21} + minutes{32} + seconds{18};
    EXPECT_EQ(*timestamp, expected);
}

TEST(LogTimeFilter, ParsesPlainFilterInput) {
    const auto expected = sys_days{year{2026} / November / day{4}} + hours{23} + minutes{18};
    EXPECT_EQ(parseLogTimestamp("04.11.2026 - 23:18:00"), expected);
    EXPECT_EQ(parseLogTimestamp("  04.11.2026 - 23:18:00  "), expected);
    EXPECT_FALSE(parseLogTimestamp("04.11.2026 - 23:18:00 extra"));
}

TEST(LogTimeFilter, RejectsMalformedEntries) {
    EXPECT_FALSE(parseLogTimestamp(""));
    EXPECT_FALSE(parseLogTimestamp("no timestamp at all"));
    EXPECT_FALSE(parseLogTimestamp("[32.13.2026 - 21:32:18] out of range"));
    EXPECT_FALSE(parseLogTimestamp("[04.11.2026 21:32:18] missing dash"));
    EXPECT_FALSE(parseLogTimestamp("[04.11.2026 - 24:00:00] bad hour"));
    EXPECT_FALSE(parseLogTimestamp("[04.11.2026 - 21:32:18 missing bracket"));
}

TEST(LogTimeFilter, RangeIsInclusive) {
    const auto timestamp = sys_days{year{2026} / November / day{4}} + hours{12};
    EXPECT_TRUE(inTimeRange(timestamp, std::nullopt, std::nullopt));
    EXPECT_TRUE(inTimeRange(timestamp, timestamp, timestamp));
    EXPECT_FALSE(inTimeRange(timestamp, timestamp + seconds{1}, std::nullopt));
    EXPECT_FALSE(inTimeRange(timestamp, std::nullopt, timestamp - seconds{1}));
}
