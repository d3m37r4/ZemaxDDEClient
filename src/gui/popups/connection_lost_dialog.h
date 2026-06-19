#pragma once

#include <string>

namespace gui {
    class ConnectionLostDialog {
        public:
            void open(const std::string& reason);
            void close() noexcept;
            [[nodiscard]] bool isOpen() const noexcept { return m_open; }

            void render();

        private:
            bool m_open = false;
            std::string m_reason;
    };
}
