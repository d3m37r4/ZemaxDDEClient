#include "app/settings.h"

#include <algorithm>
#include <fstream>
#include <limits>

#include <nlohmann/json.hpp>

namespace app {

    namespace {
        constexpr int kMinTimeoutMs = 1000;
        constexpr int kMaxTimeoutMs = 30000;
        constexpr int kMinRequestTimeoutMs = 100;
        constexpr int kMaxRequestTimeoutMs = 30000;
        constexpr int kMinRetries = 1;
        constexpr int kMaxRetries = 10;
        constexpr int kMinConnections = 1;
        constexpr int kMaxConnections = 4;
        constexpr float kMinLineWeight = 1.0f;
        constexpr float kMaxLineWeight = 3.0f;
        constexpr float kMinMarkerSize = 0.0f;
        constexpr float kMaxMarkerSize = 10.0f;

        template <typename T>
        T clampValue(T value, T min, T max, T fallback) {
            if (value < min || value > max) return fallback;
            return value;
        }
    }

    std::string_view themeModeToString(ThemeMode mode) noexcept {
        switch (mode) {
            case ThemeMode::Light:  return "light";
            case ThemeMode::Dark:   return "dark";
            case ThemeMode::System: return "system";
        }
        return "system";
    }

    ThemeMode themeModeFromString(std::string_view str) noexcept {
        if (str == "light") return ThemeMode::Light;
        if (str == "dark")  return ThemeMode::Dark;
        return ThemeMode::System;
    }

    std::string_view updateChannelToString(UpdateChannel channel) noexcept {
        switch (channel) {
            case UpdateChannel::Stable: return "stable";
            case UpdateChannel::Beta:   return "beta";
        }
        return "stable";
    }

    UpdateChannel updateChannelFromString(std::string_view str) noexcept {
        if (str == "beta") return UpdateChannel::Beta;
        return UpdateChannel::Stable;
    }

    AppSettings AppSettings::defaults() {
        AppSettings s;
        s.reset();
        return s;
    }

    void AppSettings::reset() {
        version = kCurrentVersion;

        general.showDebugLogOnStartup = true;
        general.restoreWindowLayout = true;

        appearance.themeMode = ThemeMode::System;

        dde.connectionTimeoutMs = 5000;
        dde.maxRetryCount = 3;
        dde.maxConnections = 4;

        dde.getNameTimeoutMs = 2000;
        dde.getFileTimeoutMs = 2000;
        dde.getSystemTimeoutMs = 2000;
        dde.getFieldTimeoutMs = 2000;
        dde.getWaveTimeoutMs = 2000;
        dde.getSurfaceDataProfileTimeoutMs = 2000;
        dde.getSagProfileTimeoutMs = 1000;
        dde.getSurfaceDataMapTimeoutMs = 2000;
        dde.getSagMapTimeoutMs = 1000;

        plot.showGridByDefault = true;
        plot.lineWeight = 1.0f;
        plot.markerSize = 5.0f;

        updates.autoCheckOnStartup = false;
        updates.channel = UpdateChannel::Stable;
    }

    bool AppSettings::loadFromFile(const std::string& path) {
        std::string dummy;
        return loadFromFileWithReason(path, dummy) == LoadResult::Success;
    }

    AppSettings::LoadResult AppSettings::loadFromFileWithReason(const std::string& path, std::string& errorOut) noexcept {
        errorOut.clear();

        std::ifstream in(path);
        if (!in.is_open()) return LoadResult::FileMissing;

        nlohmann::json j;
        try {
            in >> j;
        } catch (const nlohmann::json::parse_error& e) {
            errorOut = e.what();
            return LoadResult::ParseError;
        }
        if (!j.is_object()) {
            errorOut = "root JSON value is not an object";
            return LoadResult::NotAnObject;
        }

        // Unknown version → fall back to defaults (forward-incompatible).
        if (j.contains("version") && j["version"].is_number_integer()) {
            int v = j["version"].get<int>();
            if (v != kCurrentVersion) {
                errorOut = std::format("file version {} != supported version {}", v, kCurrentVersion);
                reset();
                return LoadResult::UnknownVersion;
            }
        }

        // --- General ---
        if (j.contains("general") && j["general"].is_object()) {
            const auto& g = j["general"];
            if (g.contains("showDebugLogOnStartup") && g["showDebugLogOnStartup"].is_boolean())
                general.showDebugLogOnStartup = g["showDebugLogOnStartup"].get<bool>();
            if (g.contains("restoreWindowLayout") && g["restoreWindowLayout"].is_boolean())
                general.restoreWindowLayout = g["restoreWindowLayout"].get<bool>();
        }

        // --- Appearance ---
        if (j.contains("appearance") && j["appearance"].is_object()) {
            const auto& a = j["appearance"];
            if (a.contains("themeMode") && a["themeMode"].is_string())
                appearance.themeMode = themeModeFromString(a["themeMode"].get<std::string_view>());
        }

        // --- DDE ---
        if (j.contains("dde") && j["dde"].is_object()) {
            const auto& d = j["dde"];
            if (d.contains("connectionTimeoutMs") && d["connectionTimeoutMs"].is_number_integer()) {
                dde.connectionTimeoutMs = clampValue(
                    d["connectionTimeoutMs"].get<int>(), kMinTimeoutMs, kMaxTimeoutMs, 5000);
            }
            if (d.contains("maxRetryCount") && d["maxRetryCount"].is_number_integer()) {
                dde.maxRetryCount = clampValue(
                    d["maxRetryCount"].get<int>(), kMinRetries, kMaxRetries, 3);
            }
            if (d.contains("maxConnections") && d["maxConnections"].is_number_integer()) {
                dde.maxConnections = clampValue(
                    d["maxConnections"].get<int>(), kMinConnections, kMaxConnections, 4);
            }
            if (d.contains("getNameTimeoutMs") && d["getNameTimeoutMs"].is_number_integer()) {
                dde.getNameTimeoutMs = clampValue(
                    d["getNameTimeoutMs"].get<int>(), kMinRequestTimeoutMs, kMaxRequestTimeoutMs, 2000);
            }
            if (d.contains("getFileTimeoutMs") && d["getFileTimeoutMs"].is_number_integer()) {
                dde.getFileTimeoutMs = clampValue(
                    d["getFileTimeoutMs"].get<int>(), kMinRequestTimeoutMs, kMaxRequestTimeoutMs, 2000);
            }
            if (d.contains("getSystemTimeoutMs") && d["getSystemTimeoutMs"].is_number_integer()) {
                dde.getSystemTimeoutMs = clampValue(
                    d["getSystemTimeoutMs"].get<int>(), kMinRequestTimeoutMs, kMaxRequestTimeoutMs, 2000);
            }
            if (d.contains("getFieldTimeoutMs") && d["getFieldTimeoutMs"].is_number_integer()) {
                dde.getFieldTimeoutMs = clampValue(
                    d["getFieldTimeoutMs"].get<int>(), kMinRequestTimeoutMs, kMaxRequestTimeoutMs, 2000);
            }
            if (d.contains("getWaveTimeoutMs") && d["getWaveTimeoutMs"].is_number_integer()) {
                dde.getWaveTimeoutMs = clampValue(
                    d["getWaveTimeoutMs"].get<int>(), kMinRequestTimeoutMs, kMaxRequestTimeoutMs, 2000);
            }
            // Per-window GetSurfaceData/GetSag with backward compat.
            // Legacy: single getSurfaceDataTimeoutMs / getSagTimeoutMs shared by all windows.
            auto loadPerWindow = [&](const char* newKey, int& target, int fallback) {
                if (d.contains(newKey) && d[newKey].is_number_integer()) {
                    target = clampValue(d[newKey].get<int>(), kMinRequestTimeoutMs, kMaxRequestTimeoutMs, fallback);
                }
            };
            loadPerWindow("getSurfaceDataProfileTimeoutMs", dde.getSurfaceDataProfileTimeoutMs, 2000);
            loadPerWindow("getSagProfileTimeoutMs",        dde.getSagProfileTimeoutMs,        1000);
            loadPerWindow("getSurfaceDataMapTimeoutMs",     dde.getSurfaceDataMapTimeoutMs,     2000);
            loadPerWindow("getSagMapTimeoutMs",             dde.getSagMapTimeoutMs,             1000);

            // Backward compat: if legacy fields exist but new ones don't, use legacy values.
            if (d.contains("getSurfaceDataTimeoutMs") && d["getSurfaceDataTimeoutMs"].is_number_integer()) {
                int legacy = clampValue(d["getSurfaceDataTimeoutMs"].get<int>(), kMinRequestTimeoutMs, kMaxRequestTimeoutMs, 2000);
                if (!d.contains("getSurfaceDataProfileTimeoutMs"))
                    dde.getSurfaceDataProfileTimeoutMs = legacy;
                if (!d.contains("getSurfaceDataMapTimeoutMs"))
                    dde.getSurfaceDataMapTimeoutMs = legacy;
            }
            if (d.contains("getSagTimeoutMs") && d["getSagTimeoutMs"].is_number_integer()) {
                int legacy = clampValue(d["getSagTimeoutMs"].get<int>(), kMinRequestTimeoutMs, kMaxRequestTimeoutMs, 1000);
                if (!d.contains("getSagProfileTimeoutMs"))
                    dde.getSagProfileTimeoutMs = legacy;
                if (!d.contains("getSagMapTimeoutMs"))
                    dde.getSagMapTimeoutMs = legacy;
            }
        }

        // --- Plot ---
        if (j.contains("plot") && j["plot"].is_object()) {
            const auto& p = j["plot"];
            if (p.contains("showGridByDefault") && p["showGridByDefault"].is_boolean())
                plot.showGridByDefault = p["showGridByDefault"].get<bool>();
            if (p.contains("lineWeight") && p["lineWeight"].is_number()) {
                plot.lineWeight = clampValue(
                    p["lineWeight"].get<float>(), kMinLineWeight, kMaxLineWeight, 1.0f);
            }
            if (p.contains("markerSize") && p["markerSize"].is_number()) {
                plot.markerSize = clampValue(
                    p["markerSize"].get<float>(), kMinMarkerSize, kMaxMarkerSize, 5.0f);
            }
        }

        // --- Updates ---
        if (j.contains("updates") && j["updates"].is_object()) {
            const auto& u = j["updates"];
            if (u.contains("autoCheckOnStartup") && u["autoCheckOnStartup"].is_boolean())
                updates.autoCheckOnStartup = u["autoCheckOnStartup"].get<bool>();
            if (u.contains("channel") && u["channel"].is_string())
                updates.channel = updateChannelFromString(u["channel"].get<std::string_view>());
        }

        return LoadResult::Success;
    }

    bool AppSettings::saveToFile(const std::string& path) const {
        nlohmann::json j;
        j["version"] = version;

        j["general"]["showDebugLogOnStartup"] = general.showDebugLogOnStartup;
        j["general"]["restoreWindowLayout"]   = general.restoreWindowLayout;

        j["appearance"]["themeMode"]      = std::string{themeModeToString(appearance.themeMode)};

        j["dde"]["connectionTimeoutMs"] = dde.connectionTimeoutMs;
        j["dde"]["maxRetryCount"]       = dde.maxRetryCount;
        j["dde"]["maxConnections"]      = dde.maxConnections;

        j["dde"]["getNameTimeoutMs"]              = dde.getNameTimeoutMs;
        j["dde"]["getFileTimeoutMs"]              = dde.getFileTimeoutMs;
        j["dde"]["getSystemTimeoutMs"]            = dde.getSystemTimeoutMs;
        j["dde"]["getFieldTimeoutMs"]             = dde.getFieldTimeoutMs;
        j["dde"]["getWaveTimeoutMs"]              = dde.getWaveTimeoutMs;
        j["dde"]["getSurfaceDataProfileTimeoutMs"] = dde.getSurfaceDataProfileTimeoutMs;
        j["dde"]["getSagProfileTimeoutMs"]         = dde.getSagProfileTimeoutMs;
        j["dde"]["getSurfaceDataMapTimeoutMs"]     = dde.getSurfaceDataMapTimeoutMs;
        j["dde"]["getSagMapTimeoutMs"]             = dde.getSagMapTimeoutMs;

        j["plot"]["showGridByDefault"] = plot.showGridByDefault;
        j["plot"]["lineWeight"]        = plot.lineWeight;
        j["plot"]["markerSize"]        = plot.markerSize;

        j["updates"]["autoCheckOnStartup"] = updates.autoCheckOnStartup;
        j["updates"]["channel"]            = std::string{updateChannelToString(updates.channel)};

        std::ofstream out(path, std::ios::trunc);
        if (!out.is_open()) return false;
        out << j.dump(2);
        return out.good();
    }

} // namespace app
