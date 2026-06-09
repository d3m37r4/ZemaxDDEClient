#pragma once

namespace gui {
    class AboutDialog {
        public:
            void open() noexcept;
            void close() noexcept;
            [[nodiscard]] bool isOpen() const noexcept { return m_open; }

            void render();

        private:
            bool m_open = false;
    };
}