#pragma once

#include <memory>

#include "app/settings.h"
#include "gui/popups/reset_confirm_dialog.h"

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

        private:
            enum class Section : int {
                General       = 0,
                Appearance    = 1,
                DDE           = 2,
                DDEPerformance = 3,
                PlotSettings  = 4,
                Updates       = 5,
                Files         = 6,
                Count         = 7,
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
            void renderSectionFiles();

            void applyWorkingTheme() const;
            void applyWorkingDDE() const;
            void applyWorkingPlot() const;
            void applyWorkingMap() const;

            void onSave();
            void onCancel();
            void onReset();

            SettingsManager& m_settings;
            std::unique_ptr<ResetConfirmDialog> m_resetConfirmDialog;
            app::AppSettings m_working;
            app::AppSettings m_loaded;
            Section m_section = Section::General;
            float m_sidebarWidth = 0.0f;
            bool m_open = false;
    };

} // namespace gui
