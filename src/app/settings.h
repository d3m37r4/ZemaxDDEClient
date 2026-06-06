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

    struct GeneralSettings {
        bool showDebugLogOnStartup = true;
        bool restoreWindowLayout = true;
    };

    struct AppearanceSettings {
        ThemeMode themeMode = ThemeMode::System;
    };

    struct DDESettings {
        int connectionTimeoutMs = 5000;
        int maxRetryCount = 3;
        bool autoReconnect = true;
        int maxConnections = 4;
    };

    struct PlotSettings {
        bool showGridByDefault = true;
        float lineWeight = 1.0f;
        float markerSize = 5.0f;
    };

    struct UpdateSettings {
        bool autoCheckOnStartup = false;
        UpdateChannel channel = UpdateChannel::Stable;
    };

    struct AppSettings {
        static constexpr int kCurrentVersion = 1;

        int version = kCurrentVersion;
        GeneralSettings general;
        AppearanceSettings appearance;
        DDESettings dde;
        PlotSettings plot;
        UpdateSettings updates;

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
