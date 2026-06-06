#include "app/settings.h"

#include <algorithm>
#include <fstream>
#include <limits>

#include <nlohmann/json.hpp>

namespace app {

    namespace {
        constexpr int kMinTimeoutMs = 1000;
        constexpr int kMaxTimeoutMs = 30000;
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
        dde.autoReconnect = true;
        dde.maxConnections = 4;

        plot.showGridByDefault = true;
        plot.lineWeight = 1.0f;
        plot.markerSize = 5.0f;

        updates.autoCheckOnStartup = false;
        updates.channel = UpdateChannel::Stable;
    }

    bool AppSettings::loadFromFile(const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) return false;

        nlohmann::json j;
        try {
            in >> j;
        } catch (const nlohmann::json::parse_error&) {
            return false;
        }
        if (!j.is_object()) return false;

        // Unknown version → fall back to defaults (forward-incompatible).
        if (j.contains("version") && j["version"].is_number_integer()) {
            int v = j["version"].get<int>();
            if (v != kCurrentVersion) {
                reset();
                return true;
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
            if (d.contains("autoReconnect") && d["autoReconnect"].is_boolean())
                dde.autoReconnect = d["autoReconnect"].get<bool>();
            if (d.contains("maxConnections") && d["maxConnections"].is_number_integer()) {
                dde.maxConnections = clampValue(
                    d["maxConnections"].get<int>(), kMinConnections, kMaxConnections, 4);
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

        return true;
    }

    bool AppSettings::saveToFile(const std::string& path) const {
        nlohmann::json j;
        j["version"] = version;

        j["general"]["showDebugLogOnStartup"] = general.showDebugLogOnStartup;
        j["general"]["restoreWindowLayout"]   = general.restoreWindowLayout;

        j["appearance"]["themeMode"]      = std::string{themeModeToString(appearance.themeMode)};

        j["dde"]["connectionTimeoutMs"] = dde.connectionTimeoutMs;
        j["dde"]["maxRetryCount"]       = dde.maxRetryCount;
        j["dde"]["autoReconnect"]       = dde.autoReconnect;
        j["dde"]["maxConnections"]      = dde.maxConnections;

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
