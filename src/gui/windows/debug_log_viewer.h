#pragma once

class Logger;

namespace gui {
    class DebugLogViewer {
        public:
            void render(Logger& logger, bool* pOpen = nullptr);
    };
}
