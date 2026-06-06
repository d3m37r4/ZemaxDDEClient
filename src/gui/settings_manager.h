#pragma once

#include "app/settings.h"
#include "gui/theme_manager.h"

class DDEConnectionManager;

namespace gui {

    /// Centralized bridge between persistent AppSettings and runtime subsystems.
    /// apply() pushes a complete settings snapshot to all bound consumers;
    /// per-section methods (applyTheme / applyDDE / applyPlot / applyUpdates) exist
    /// for the Preferences dialog's live preview.
    class SettingsManager {
        public:
            SettingsManager() = default;
            ~SettingsManager() = default;

            SettingsManager(const SettingsManager&) = delete;
            SettingsManager& operator=(const SettingsManager&) = delete;

            // Reads the system theme from the Windows registry. Used by ThemeMode::System.
            static bool detectSystemDarkMode();

            // Non-owning bindings; called once during GuiManager wiring.
            void bind(ThemeManager* themeManager, DDEConnectionManager* ddeConnectionManager);

            // Applies every section in @p settings. Requires bind() to have been called
            // for the consumers used by the settings being applied.
            void apply(const app::AppSettings& settings);

            // Per-section apply methods. Safe to call individually for live preview.
            // Each method uses detectSystemDarkMode() internally for theme decisions.
            void applyTheme(const app::AppearanceSettings& appearance);
            void applyDDE(const app::DDESettings& dde);
            void applyPlot(const app::PlotSettings& plot);
            void applyUpdates(const app::UpdateSettings& updates);

            [[nodiscard]] const app::AppSettings& current() const noexcept { return m_current; }

        private:
            app::AppSettings m_current;
            ThemeManager* m_themeManager = nullptr;
            DDEConnectionManager* m_ddeConnectionManager = nullptr;
    };

} // namespace gui
