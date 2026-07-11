#pragma once

#include <memory>
#include <string>

#include "app/settings.h"
#include "gui/popups/reset_confirm_dialog.h"

class Logger;

namespace gui {

    class SettingsManager;

    /// Resizable modal dialog with 7 sections (General, Appearance, DDE, DDE Performance, Plot, Updates, Files)
    /// plus Save/Cancel/Reset actions. Uses a "working copy" of AppSettings for live preview:
    /// per-section apply methods on SettingsManager are called as soon as the user mutates
    /// a value, and the cancel button restores the last-saved snapshot.
    class PreferencesDialog {
        public:
            explicit PreferencesDialog(SettingsManager& settings) noexcept;

            void open() noexcept;
            void close() noexcept;
            [[nodiscard]] bool isOpen() const noexcept { return m_open; }

            // Must be called every frame from the main GUI loop while the dialog is open.
            void render();

            void setActiveLogPath(const std::string& path) { m_activeLogPath = path; }
            void setLogger(Logger* logger) noexcept { m_logger = logger; }

        private:
            enum class Section : int {
                General       = 0,
                Appearance    = 1,
                DDE           = 2,
                DDEPerformance = 3,
                PlotSettings  = 4,
                Updates       = 5,
                Logging       = 6,
                Files         = 7,
                Count         = 8,
            };

            void renderSidebar();
            void renderContent();
            void renderFooter();

            void renderSectionGeneral();
            void renderSectionAppearance();
            void renderSectionDDE();
            void renderSectionDDEPerformance();
            void renderSectionPlotSettings();
            void renderSectionUpdates();
            void renderSectionLogging();
            void renderSectionFiles();

            void applyWorkingTheme() const;
            void applyWorkingDDE() const;
            void applyWorkingPlot() const;
            void applyWorkingMap() const;

            void onSave();
            void onCancel();
            void onReset();
            void onCleanLogs();

            SettingsManager& m_settings;
            std::unique_ptr<ResetConfirmDialog> m_resetConfirmDialog;
            std::unique_ptr<ResetConfirmDialog> m_cleanLogsConfirmDialog;
            app::AppSettings m_working;
            app::AppSettings m_loaded;
            Section m_section = Section::General;
            float m_sidebarWidth = 0.0f;
            bool m_open = false;
            std::string m_activeLogPath;
            Logger* m_logger = nullptr;
    };

} // namespace gui
