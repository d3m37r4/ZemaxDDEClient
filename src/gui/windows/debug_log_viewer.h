#pragma once

class Logger;

namespace gui {
    /**
     * @brief Renders the debug log window with export, copy, and clear functionality.
     */
    class DebugLogViewer {
        public:
            void render(Logger& logger);
    };
}
