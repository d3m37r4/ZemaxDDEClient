#include <gtest/gtest.h>

#include "gui/log_filter_util.h"

using gui::logs::LogLevel;

TEST(LogFilter, LevelFromTag) {
    EXPECT_EQ(gui::logs::levelOf("[04.11.2026 - 21:32:18] [APP] Main loop started"), LogLevel::App);
    EXPECT_EQ(gui::logs::levelOf("[04.11.2026 - 21:32:18] [DDE] Sent request #73"), LogLevel::Dde);
    EXPECT_EQ(gui::logs::levelOf("[04.11.2026 - 21:32:18] [GUI] Log exported"), LogLevel::Gui);
    EXPECT_EQ(gui::logs::levelOf("[04.11.2026 - 21:32:18] [SurfaceProfileService] Completed"),
              LogLevel::Service);
}

TEST(LogFilter, MissingTagIsUnknown) {
    EXPECT_EQ(gui::logs::levelOf("[04.11.2026 - 21:32:18] plain text"), LogLevel::Unknown);
    EXPECT_EQ(gui::logs::levelOf("[04.11.2026 - 21:32:18] "), LogLevel::Unknown);
    EXPECT_EQ(gui::logs::levelOf("no timestamp at all"), LogLevel::Unknown);
    EXPECT_EQ(gui::logs::levelOf(""), LogLevel::Unknown);
}

TEST(LogFilter, SearchIsCaseInsensitive) {
    EXPECT_TRUE(gui::logs::containsNoCase("[DDE] Request Timeout", "timeout"));
    EXPECT_TRUE(gui::logs::containsNoCase("Hello World", "WORLD"));
    EXPECT_TRUE(gui::logs::containsNoCase("anything", ""));
    EXPECT_FALSE(gui::logs::containsNoCase("abc", "abcd"));
    EXPECT_FALSE(gui::logs::containsNoCase("abc", "z"));
}

TEST(LogFilter, ProblemWordsAreDetected) {
    EXPECT_TRUE(gui::logs::looksLikeProblem("[DDE] Request failed: timeout"));
    EXPECT_TRUE(gui::logs::looksLikeProblem("[GUI] ERROR while saving"));
    EXPECT_TRUE(gui::logs::looksLikeProblem("[APP] Task cancelled"));
    EXPECT_TRUE(gui::logs::looksLikeProblem("[DDE] request timeout after 1000 ms"));
    EXPECT_FALSE(gui::logs::looksLikeProblem("[DDE] Sent request #73"));
}

TEST(LogFilter, KeyValueParametersAreNotProblems) {
    // Regression: timeout=2000ms is a request parameter, not a failure.
    EXPECT_FALSE(gui::logs::looksLikeProblem(
        "[05.11.2026 - 18:39:17] [DDE] Submitted request #6: 'GetWave,0' "
        "(svc=InitialDataLoad, timeout=2000ms, retries=1)"));
    EXPECT_FALSE(gui::logs::looksLikeProblem("[APP] errorCode=0"));
    EXPECT_FALSE(gui::logs::looksLikeProblem("[APP] cancelPendingRequests: 0"));
}

TEST(LogFilter, SuccessWordsAreDetected) {
    EXPECT_TRUE(gui::logs::looksLikeSuccess("[APP] Data load completed: 1 fields"));
    EXPECT_TRUE(gui::logs::looksLikeSuccess("[DDE] Connected slot 0: 'LENS.ZMX'"));
    EXPECT_TRUE(gui::logs::looksLikeSuccess("[GUI] Settings saved"));
    EXPECT_TRUE(gui::logs::looksLikeSuccess("[APP] Main loop started"));
    EXPECT_FALSE(gui::logs::looksLikeSuccess("[DDE] Sent request #73"));
    // "Disconnected" must not be read as "connected": word start is required.
    EXPECT_FALSE(gui::logs::looksLikeSuccess("[GUI] Disconnected from Zemax"));
}
