#include "gui/settings_manager.h"

#include <windows.h>

#include "lib/implot/implot.h"

#include "dde/dde_connection_manager.h"
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
        // Phase 7+:  applyUpdates(settings.updates);

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
        (void)updates;
        // Implemented in Phase 6/7
    }

} // namespace gui
