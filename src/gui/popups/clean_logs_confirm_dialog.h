#pragma once

#include <functional>

class ThemeManager;

namespace gui {

    /// Modal confirmation dialog for cleaning the log folder.
    /// Owned by PreferencesDialog; triggers a callback when the user confirms.
    class CleanLogsConfirmDialog {
        public:
            using ConfirmCallback = std::function<void()>;

            void open() noexcept;
            void close() noexcept;
            [[nodiscard]] bool isOpen() const noexcept { return m_open; }

            /// Called every frame by the owning PreferencesDialog.
            void render();

            void setOnConfirm(ConfirmCallback cb) { m_onConfirm = std::move(cb); }
            void setThemeManager(ThemeManager* tm) noexcept { m_themeManager = tm; }

        private:
            bool m_open = false;
            bool m_confirm = false;
            ConfirmCallback m_onConfirm;
            ThemeManager* m_themeManager = nullptr;
    };

} // namespace gui
