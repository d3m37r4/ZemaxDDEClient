#pragma once

class Logger;
class ThemeManager;

namespace gui {
    class Logs {
    public:
        void render(Logger& logger, const ThemeManager* themeManager);
    };
}
