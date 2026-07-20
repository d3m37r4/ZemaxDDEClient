#pragma once

class Logger;
class ThemeManager;

namespace gui {
    class DebugLog {
    public:
        void render(Logger& logger, const ThemeManager* themeManager);
    };
}
