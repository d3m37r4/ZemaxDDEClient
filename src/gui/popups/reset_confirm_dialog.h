#pragma once

#include <functional>

class ThemeManager;

namespace gui {

    /// Modal confirmation dialog for resetting all preferences to factory defaults.
    /// Owned by PreferencesDialog; triggers a callback when the user confirms.
    class ResetConfirmDialog {
        public:
            using ResetCallback = std::function<void()>;

            void open() noexcept;
            void close() noexcept;
            [[nodiscard]] bool isOpen() const noexcept { return m_open; }

            /// Called every frame by the owning PreferencesDialog.
            void render();

            void setOnReset(ResetCallback cb) { m_onReset = std::move(cb); }
            void setThemeManager(ThemeManager* tm) noexcept { m_themeManager = tm; }

        private:
            bool m_open = false;
            bool m_confirmReset = false;
            ResetCallback m_onReset;
            ThemeManager* m_themeManager = nullptr;
    };

} // namespace gui
