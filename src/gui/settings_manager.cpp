#include "gui/settings_manager.h"

#include <format>

#include "app/config_path.h"
#include "lib/implot/implot.h"
#include "logger/logger.h"

#include "dde/dde_connection_manager.h"
#include "gui/popups/update_checker.h"
#include "gui/theme_manager.h"

namespace gui {

    bool SettingsManager::detectSystemDarkMode() {
        HKEY hKey = nullptr;
        LONG regResult = RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &hKey);
        if (regResult != ERROR_SUCCESS) return false;

        DWORD lightTheme = 1;
        DWORD dataSize = sizeof(DWORD);
        LONG queryResult = RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
            (LPBYTE)&lightTheme, &dataSize);
        RegCloseKey(hKey);

        if (queryResult != ERROR_SUCCESS) return false;
        return lightTheme == 0;
    }

    void SettingsManager::bind(ThemeManager* themeManager, DDEConnectionManager* ddeConnectionManager) {
        m_themeManager = themeManager;
        m_ddeConnectionManager = ddeConnectionManager;
    }

    void SettingsManager::apply(const app::AppSettings& settings) {
        applyTheme(settings.appearance);
        applyDDE(settings.dde);
        applyPlot(settings.plot);
        applyUpdates(settings.updates);

        m_current = settings;
    }

    void SettingsManager::applyTheme(const app::AppearanceSettings& appearance) {
        if (!m_themeManager) return;
        m_themeManager->applyByMode(appearance.themeMode, detectSystemDarkMode());
    }

    void SettingsManager::applyDDE(const app::DDESettings& dde) {
        if (!m_ddeConnectionManager) return;
        m_ddeConnectionManager->setDefaultTimeoutMs(static_cast<DWORD>(dde.connectionTimeoutMs));
        m_ddeConnectionManager->setDefaultRetries(dde.maxRetryCount);
        m_ddeConnectionManager->setMaxConnections(dde.maxConnections);
        m_ddeConnectionManager->setAutoReconnect(dde.autoReconnect);
    }

    void SettingsManager::applyPlot(const app::PlotSettings& plot) {
        (void)plot;
        // ImPlot 1.x does not expose LineWeight/MarkerSize as global ImPlotStyle fields;
        // they are per-item properties on ImPlotSpec. Surface services read them via
        // plotLineWeight()/plotMarkerSize() and pass to each PlotLine()/PlotScatter() call.
        // showGridByDefault is consumed by surface services via showGridByDefault().
    }

    void SettingsManager::applyUpdates(const app::UpdateSettings& updates) {
        if (!m_updateChecker) return;
        m_updateChecker->setChannel(updates.channel);
        m_updateChecker->setAutoCheckOnStartup(updates.autoCheckOnStartup);
    }

    bool SettingsManager::loadFromFile() {
        const char* path = app::getSettingsJsonPath();
        if (!path || !*path) return false;

        app::AppSettings loaded;
        std::string err;
        auto result = loaded.loadFromFileWithReason(path, err);
        if (result != app::AppSettings::LoadResult::Success) {
            if (m_logger && result != app::AppSettings::LoadResult::FileMissing) {
                std::string_view kind = "unknown";
                switch (result) {
                    case app::AppSettings::LoadResult::ParseError:     kind = "JSON parse error"; break;
                    case app::AppSettings::LoadResult::NotAnObject:    kind = "not a JSON object"; break;
                    case app::AppSettings::LoadResult::UnknownVersion: kind = "unknown version"; break;
                    default: break;
                }
                m_logger->addLog(std::format("[SETTINGS] Failed to load {} ({}): {}",
                                             path, kind, err));
            }
            m_current = loaded;
            apply(m_current);
            return false;
        }

        m_current = loaded;
        apply(m_current);
        return true;
    }

    bool SettingsManager::saveToFile() const {
        const char* path = app::getSettingsJsonPath();
        if (!path || !*path) return false;
        return m_current.saveToFile(path);
    }

} // namespace gui
