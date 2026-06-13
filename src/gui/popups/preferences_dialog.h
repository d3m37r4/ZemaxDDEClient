#pragma once

#include "app/settings.h"

namespace gui {

    class SettingsManager;

    /// Resizable modal dialog with 5 sections (General, Appearance, DDE, Plot, Updates)
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
                General     = 0,
                Appearance  = 1,
                DDE         = 2,
                Plot        = 3,
                Updates     = 4,
                Count       = 5,
            };

            void renderSidebar();
            void renderContent();
            void renderFooter();
            void renderResetConfirm();

            void renderSectionGeneral();
            void renderSectionAppearance();
            void renderSectionDDE();
            void renderSectionPlot();
            void renderSectionUpdates();

            void applyWorkingTheme() const;
            void applyWorkingDDE() const;
            void applyWorkingPlot() const;

            void onSave();
            void onCancel();
            void onReset();

            SettingsManager& m_settings;
            app::AppSettings m_working;
            app::AppSettings m_loaded;
            Section m_section = Section::General;
            float m_sidebarWidth = 0.0f;
            bool m_open = false;
            bool m_confirmReset = false;
    };

} // namespace gui
