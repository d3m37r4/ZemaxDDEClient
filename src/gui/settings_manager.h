#pragma once

#include "app/settings.h"
#include "gui/theme_manager.h"

class DDEConnectionManager;
class Logger;

namespace gui {
    class GraphicsBackend;
    class UpdateChecker;

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
            void setGraphicsBackend(GraphicsBackend* gb) noexcept { m_graphicsBackend = gb; }
            void setUpdateChecker(UpdateChecker* updateChecker) noexcept { m_updateChecker = updateChecker; }
            void setLogger(Logger* logger) noexcept { m_logger = logger; }

            // Applies every section in @p settings. Requires bind() to have been called
            // for the consumers used by the settings being applied.
            void apply(const app::AppSettings& settings);

            // File I/O for persistence. loadFromFile() reads settings.json, replaces
            // m_current and pushes the values to all bound subsystems. saveToFile()
            // writes m_current to settings.json. Both return false on I/O errors.
            bool loadFromFile();
            bool saveToFile() const;

            // Reads settings.json into m_current WITHOUT calling apply().
            // Used during startup to resolve the correct theme before
            // GraphicsBackend::initialize() applies it.
            [[nodiscard]] bool loadFromDisk();

            // Per-section apply methods. Safe to call individually for live preview.
            // Each method uses detectSystemDarkMode() internally for theme decisions.
            void applyTheme(const app::AppearanceSettings& appearance);
            void applyDDE(const app::DDESettings& dde);
            void applyPlot(const app::PlotSettings& plot);

            /// Returns the cached 'show grid by default' flag set by applyPlot().
            /// Surface profile and surface map services read this when configuring
            /// ImPlot::SetupAxes to honor the user's preference on new plots.
            [[nodiscard]] bool showGridByDefault() const noexcept { return m_current.plot.showGridByDefault; }

            /// Returns the cached line weight (pixels) for ImPlot plot lines.
            /// In ImPlot 1.x LineWeight is a per-item ImPlotSpec property, not a
            /// global style field, so surface services must pass this via ImPlotSpec
            /// to each PlotLine()/PlotScatter() call.
            [[nodiscard]] float plotLineWeight() const noexcept { return m_current.plot.lineWeight; }

            /// Returns the cached marker size (pixels) for ImPlot plot markers.
            /// Same caveat as plotLineWeight(): only takes effect where the plot item
            /// uses a marker (PlotScatter/PlotLine with Marker set in ImPlotSpec).
            [[nodiscard]] float plotMarkerSize() const noexcept { return m_current.plot.markerSize; }

            void applyUpdates(const app::UpdateSettings& updates);

            [[nodiscard]] const app::AppSettings& current() const noexcept { return m_current; }

        private:
            app::AppSettings m_current;
            ThemeManager* m_themeManager = nullptr;
            GraphicsBackend* m_graphicsBackend = nullptr;
            DDEConnectionManager* m_ddeConnectionManager = nullptr;
            UpdateChecker* m_updateChecker = nullptr;
            Logger* m_logger = nullptr;
    };

} // namespace gui
