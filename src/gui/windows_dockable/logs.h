#pragma once

class Logger;
class ThemeManager;

namespace gui {
    class Logs {
    public:
        void render(Logger& logger, const ThemeManager* themeManager);

    private:
        /// Панель одна — состояние тумблера живёт в единственном экземпляре.
        bool m_autoscroll = true;

        /// разовый запрос «прыгнуть в самый низ» (включение тумблера);
        /// исполняется в child контента, т.к. SetScrollY целялся бы в шапку.
        bool m_jumpToBottom = false;

        /// Пользователь запросил очистку лога — модалка подтверждения откроется
        /// на ближайшем кадре; флаг снимается при закрытии любым способом
        /// (Cancel / Clear / крестик / Esc).
        bool m_confirmDelete = false;
    };
}
