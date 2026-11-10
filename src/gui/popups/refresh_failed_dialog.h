#pragma once

#include <string>

namespace gui {
    /// Shown when GetRefresh reported "-1": Zemax cannot update the lens, so no
    /// reliable sag data is available and the calculation was stopped.
    /// requestOpen() is safe to call from a DDE callback; the popup itself is
    /// opened during the render phase.
    class RefreshFailedDialog {
        public:
            void requestOpen(const std::string& reason);
            void close() noexcept;
            [[nodiscard]] bool isOpen() const noexcept { return m_open; }

            void render();

        private:
            bool m_open = false;
            bool m_pending = false;
            std::string m_reason;
    };
}
