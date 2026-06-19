#pragma once

#include <functional>

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

        private:
            bool m_open = false;
            bool m_confirmReset = false;
            ResetCallback m_onReset;
    };

} // namespace gui
