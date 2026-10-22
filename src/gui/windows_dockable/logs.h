#pragma once

class Logger;
class ThemeManager;

namespace gui {
    class Logs {
    public:
        void render(Logger& logger, const ThemeManager* themeManager);

    private:
        // Single panel - the toggle state lives in one instance.
        bool m_autoscroll = true;

        // One-shot "jump to the very bottom" request (enabling the toggle);
        // executed in the content child, since SetScrollY would target the header.
        bool m_jumpToBottom = false;

        // User requested a log clear - the confirmation modal opens on the
        // nearest frame; the flag is cleared when the modal closes in any way
        // (Cancel / Clear / close button / Esc).
        bool m_confirmDelete = false;
    };
}
