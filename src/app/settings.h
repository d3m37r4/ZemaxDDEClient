#pragma once

#include <string>
#include <string_view>

namespace app {

    enum class ThemeMode {
        Light,
        Dark,
        System,
    };

    enum class UpdateChannel {
        Stable,
        Beta,
    };

    // Interface font scale bounds (multiplier over BASE_FONT_SIZE).
    inline constexpr float kDefaultFontScale = 1.0f;
    inline constexpr float kMinFontScale = 0.75f;
    inline constexpr float kMaxFontScale = 3.0f;

    // Numeric bounds of the persisted settings. The loader clamps stored values
    // and the Preferences fields clamp live edits to the same ranges.
    inline constexpr int kMinTimeoutMs = 1000;
    inline constexpr int kMaxTimeoutMs = 30000;
    inline constexpr int kMinRequestTimeoutMs = 100;
    inline constexpr int kMaxRequestTimeoutMs = 30000;
    inline constexpr int kMinRetries = 0;
    inline constexpr int kMaxRetries = 10;
    inline constexpr int kMinConnections = 1;
    /// Must match DDEConnectionManager::MAX_CONNECTIONS (asserted in
    /// settings_manager.cpp): the application supports a fixed pair of slots.
    inline constexpr int kMaxConnections = 2;
    inline constexpr float kMinLineWeight = 1.0f;
    inline constexpr float kMaxLineWeight = 3.0f;
    inline constexpr float kMinMarkerSize = 0.0f;
    inline constexpr float kMaxMarkerSize = 10.0f;
    inline constexpr int kMinFileSizeMB = 1;
    inline constexpr int kMaxFileSizeMB = 100;
    inline constexpr int kMinFolderThresholdMB = 10;
    inline constexpr int kMaxFolderThresholdMB = 500;

    struct GeneralSettings {
        bool restoreWindowLayout = true;
        /// Show the Welcome card at application start. The card has no View
        /// menu entry: closing it hides it for the rest of the session.
        bool showWelcomeOnStartup = true;
        /// Apply the saved default layout (default_layout.json) at startup
        /// instead of the previous session's layout.
        bool applyDefaultLayoutOnStartup = false;
    };

    struct AppearanceSettings {
        ThemeMode themeMode = ThemeMode::System;
        // Interface font scale multiplier over BASE_FONT_SIZE (1.0 = 100%).
        // Applied via ImGui::GetIO().FontGlobalScale; clamped on load.
        float fontScale = kDefaultFontScale;
    };

    struct DDESettings {
        int connectionTimeoutMs = 5000;
        int maxRetryCount = 3;
        /// Effective pool size; clamped to DDEConnectionManager::MAX_CONNECTIONS.
        int maxConnections = 2;

        // Per-request timeout overrides — Initial Data Load (startup).
        int getNameTimeoutMs = 2000;
        int getFileTimeoutMs = 2000;
        int getSystemTimeoutMs = 2000;
        int getFieldTimeoutMs = 2000;
        int getWaveTimeoutMs = 2000;

        // GetRefresh issued before reading a lens (once per user action).
        int getRefreshTimeoutMs = 2000;

        // Per-request timeout overrides — Surface Profile Inspector.
        int getSurfaceDataProfileTimeoutMs = 2000;
        int getSagProfileTimeoutMs = 1000;

        // Per-request timeout overrides — Surface Irregularity Map.
        int getSurfaceDataMapTimeoutMs = 2000;
        int getSagMapTimeoutMs = 1000;
    };

    struct PlotSettings {
        bool showGridByDefault = true;
        float lineWeight = 1.0f;
        float markerSize = 5.0f;
    };

    struct MapSettings {
        int defaultColormapSurface = 0;
        int defaultColormapDeviation = 0;
        bool highlightWorstSurface = true;
        bool highlightWorstDeviation = true;
        float worstColorSurface[3] = {1.0f, 0.0f, 0.0f};
        float worstColorDeviation[3] = {1.0f, 0.0f, 0.0f};
    };

    struct UpdateSettings {
        bool autoCheckOnStartup = false;
        UpdateChannel channel = UpdateChannel::Stable;
    };

    struct LoggingSettings {
        bool showLogsOnStartup = true;
        bool enableFileLogging = false;
        int logFileSizeMB = 5;
        int logFolderSizeThresholdMB = 50;
    };

    struct AppSettings {
        static constexpr int kCurrentVersion = 1;

        int version = kCurrentVersion;
        GeneralSettings general;
        AppearanceSettings appearance;
        DDESettings dde;
        PlotSettings plot;
        MapSettings map;
        UpdateSettings updates;
        LoggingSettings logging;

        void reset();
        [[nodiscard]] static AppSettings defaults();

        // Outcome of loadFromFileWithReason. FileMissing is NOT an error
        // (first launch), all other non-Success values indicate a real
        // problem that should be reported to the user.
        enum class LoadResult {
            Success,
            FileMissing,
            ParseError,
            NotAnObject,
            UnknownVersion,
        };

        // Throws std::runtime_error if file missing and createIfMissing==false,
        // returns false on parse errors. Logs to logger; never throws from JSON layer.
        [[nodiscard]] bool loadFromFile(const std::string& path);
        [[nodiscard]] LoadResult loadFromFileWithReason(const std::string& path, std::string& errorOut) noexcept;
        [[nodiscard]] bool saveToFile(const std::string& path) const;
    };

    // Enum <-> string helpers (used by JSON load/save).
    [[nodiscard]] std::string_view themeModeToString(ThemeMode mode) noexcept;
    [[nodiscard]] ThemeMode themeModeFromString(std::string_view str) noexcept;

    [[nodiscard]] std::string_view updateChannelToString(UpdateChannel channel) noexcept;
    [[nodiscard]] UpdateChannel updateChannelFromString(std::string_view str) noexcept;

} // namespace app
