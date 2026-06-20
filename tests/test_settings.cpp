#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <string>

#include "app/settings.h"

namespace app {
namespace {

// ============================================================
//  Helpers
// ============================================================

static std::string tmpPath(const char* name) {
    return std::string(std::tmpnam(nullptr)) + name;
}

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::trunc);
    out << content;
}

// ============================================================
//  AppSettings::defaults() / reset()
// ============================================================

TEST(SettingsDefaults, AllFieldsMatchExpected) {
    auto s = AppSettings::defaults();

    EXPECT_EQ(s.version, AppSettings::kCurrentVersion);
    EXPECT_TRUE(s.general.showDebugLogOnStartup);
    EXPECT_TRUE(s.general.restoreWindowLayout);
    EXPECT_EQ(s.appearance.themeMode, ThemeMode::System);
    EXPECT_EQ(s.dde.connectionTimeoutMs, 5000);
    EXPECT_EQ(s.dde.maxRetryCount, 3);
    EXPECT_EQ(s.dde.maxConnections, 4);
    EXPECT_EQ(s.dde.getNameTimeoutMs, 2000);
    EXPECT_EQ(s.dde.getFileTimeoutMs, 2000);
    EXPECT_EQ(s.dde.getSystemTimeoutMs, 2000);
    EXPECT_EQ(s.dde.getFieldTimeoutMs, 2000);
    EXPECT_EQ(s.dde.getWaveTimeoutMs, 2000);
    EXPECT_EQ(s.dde.getSurfaceDataProfileTimeoutMs, 2000);
    EXPECT_EQ(s.dde.getSagProfileTimeoutMs, 1000);
    EXPECT_EQ(s.dde.getSurfaceDataMapTimeoutMs, 2000);
    EXPECT_EQ(s.dde.getSagMapTimeoutMs, 1000);
    EXPECT_TRUE(s.plot.showGridByDefault);
    EXPECT_FLOAT_EQ(s.plot.lineWeight, 1.0f);
    EXPECT_FLOAT_EQ(s.plot.markerSize, 5.0f);
    EXPECT_FALSE(s.updates.autoCheckOnStartup);
    EXPECT_EQ(s.updates.channel, UpdateChannel::Stable);
}

TEST(SettingsReset, RestoresDefaults) {
    AppSettings s;
    s.dde.connectionTimeoutMs = 9999;
    s.appearance.themeMode = ThemeMode::Light;
    s.plot.lineWeight = 2.5f;

    s.reset();

    EXPECT_EQ(s.dde.connectionTimeoutMs, 5000);
    EXPECT_EQ(s.appearance.themeMode, ThemeMode::System);
    EXPECT_FLOAT_EQ(s.plot.lineWeight, 1.0f);
}

// ============================================================
//  Enum roundtrip
// ============================================================

TEST(ThemeModeRoundtrip, LightDarkSystem) {
    EXPECT_EQ(themeModeFromString(themeModeToString(ThemeMode::Light)),  ThemeMode::Light);
    EXPECT_EQ(themeModeFromString(themeModeToString(ThemeMode::Dark)),   ThemeMode::Dark);
    EXPECT_EQ(themeModeFromString(themeModeToString(ThemeMode::System)), ThemeMode::System);
}

TEST(ThemeModeFromString, InvalidReturnsSystem) {
    EXPECT_EQ(themeModeFromString("invalid"), ThemeMode::System);
    EXPECT_EQ(themeModeFromString(""), ThemeMode::System);
}

TEST(UpdateChannelRoundtrip, StableBeta) {
    EXPECT_EQ(updateChannelFromString(updateChannelToString(UpdateChannel::Stable)), UpdateChannel::Stable);
    EXPECT_EQ(updateChannelFromString(updateChannelToString(UpdateChannel::Beta)),   UpdateChannel::Beta);
}

TEST(UpdateChannelFromString, InvalidReturnsStable) {
    EXPECT_EQ(updateChannelFromString("invalid"), UpdateChannel::Stable);
    EXPECT_EQ(updateChannelFromString(""), UpdateChannel::Stable);
}

// ============================================================
//  Load / save roundtrip
// ============================================================

TEST(SettingsLoadSave, RoundtripPreservesValues) {
    std::string path = tmpPath("_settings_roundtrip.json");

    AppSettings s;
    s.dde.connectionTimeoutMs = 7777;
    s.appearance.themeMode = ThemeMode::Dark;
    s.plot.lineWeight = 2.0f;
    s.updates.channel = UpdateChannel::Beta;
    ASSERT_TRUE(s.saveToFile(path));

    AppSettings loaded;
    std::string err;
    EXPECT_EQ(loaded.loadFromFileWithReason(path, err), AppSettings::LoadResult::Success);
    EXPECT_EQ(loaded.dde.connectionTimeoutMs, 7777);
    EXPECT_EQ(loaded.appearance.themeMode, ThemeMode::Dark);
    EXPECT_FLOAT_EQ(loaded.plot.lineWeight, 2.0f);
    EXPECT_EQ(loaded.updates.channel, UpdateChannel::Beta);

    std::remove(path.c_str());
}

// ============================================================
//  Load error cases
// ============================================================

TEST(SettingsLoad, FileMissingReturnsFileMissing) {
    AppSettings s;
    std::string err;
    EXPECT_EQ(s.loadFromFileWithReason("nonexistent_file_abc123.json", err), AppSettings::LoadResult::FileMissing);
}

TEST(SettingsLoad, ParseErrorReturnsParseError) {
    std::string path = tmpPath("_settings_parse_err.json");
    writeFile(path, "{invalid json!!!}");

    AppSettings s;
    std::string err;
    EXPECT_EQ(s.loadFromFileWithReason(path, err), AppSettings::LoadResult::ParseError);
    EXPECT_FALSE(err.empty());

    std::remove(path.c_str());
}

TEST(SettingsLoad, NotAnObjectReturnsNotAnObject) {
    std::string path = tmpPath("_settings_notobj.json");
    writeFile(path, "[1, 2, 3]");

    AppSettings s;
    std::string err;
    EXPECT_EQ(s.loadFromFileWithReason(path, err), AppSettings::LoadResult::NotAnObject);

    std::remove(path.c_str());
}

TEST(SettingsLoad, UnknownVersionResetsAndReturns) {
    std::string path = tmpPath("_settings_ver.json");
    writeFile(path, R"({"version": 999})");

    AppSettings s;
    s.dde.connectionTimeoutMs = 7777;
    std::string err;
    EXPECT_EQ(s.loadFromFileWithReason(path, err), AppSettings::LoadResult::UnknownVersion);
    EXPECT_FALSE(err.empty());

    std::remove(path.c_str());
}

// ============================================================
//  DDE clamp
// ============================================================

TEST(SettingsClamp, ConnectionTimeoutBelowMin) {
    std::string path = tmpPath("_settings_clamp_min.json");
    writeFile(path, R"({"version": 1, "dde": {"connectionTimeoutMs": 100}})");

    AppSettings s;
    std::string err;
    ASSERT_EQ(s.loadFromFileWithReason(path, err), AppSettings::LoadResult::Success);
    EXPECT_EQ(s.dde.connectionTimeoutMs, 5000);

    std::remove(path.c_str());
}

TEST(SettingsClamp, ConnectionTimeoutAboveMax) {
    std::string path = tmpPath("_settings_clamp_max.json");
    writeFile(path, R"({"version": 1, "dde": {"connectionTimeoutMs": 99999}})");

    AppSettings s;
    std::string err;
    ASSERT_EQ(s.loadFromFileWithReason(path, err), AppSettings::LoadResult::Success);
    EXPECT_EQ(s.dde.connectionTimeoutMs, 5000);

    std::remove(path.c_str());
}

TEST(SettingsClamp, ConnectionTimeoutValid) {
    std::string path = tmpPath("_settings_clamp_valid.json");
    writeFile(path, R"({"version": 1, "dde": {"connectionTimeoutMs": 3000}})");

    AppSettings s;
    std::string err;
    ASSERT_EQ(s.loadFromFileWithReason(path, err), AppSettings::LoadResult::Success);
    EXPECT_EQ(s.dde.connectionTimeoutMs, 3000);

    std::remove(path.c_str());
}

TEST(SettingsClamp, LineWeightBelowMin) {
    std::string path = tmpPath("_settings_lw.json");
    writeFile(path, R"({"version": 1, "plot": {"lineWeight": 0.1}})");

    AppSettings s;
    std::string err;
    ASSERT_EQ(s.loadFromFileWithReason(path, err), AppSettings::LoadResult::Success);
    EXPECT_FLOAT_EQ(s.plot.lineWeight, 1.0f);

    std::remove(path.c_str());
}

TEST(SettingsClamp, MarkerSizeAboveMax) {
    std::string path = tmpPath("_settings_ms.json");
    writeFile(path, R"({"version": 1, "plot": {"markerSize": 50.0}})");

    AppSettings s;
    std::string err;
    ASSERT_EQ(s.loadFromFileWithReason(path, err), AppSettings::LoadResult::Success);
    EXPECT_FLOAT_EQ(s.plot.markerSize, 5.0f);

    std::remove(path.c_str());
}

} // namespace
} // namespace app
