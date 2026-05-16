#pragma once

#include <functional>
class Logger;
class DDEConnectionManager;
class WindowManager;

namespace gui {
    /**
     * @brief Handles rendering of the top navigation bar.
     */
    class MenuBarController {
        public:
            explicit MenuBarController(Logger& logger, DDEConnectionManager* ddeMgr);
            void render();
            void setExitCallback(std::function<void()> cb);
            void setAboutCallback(std::function<void()> cb);
            void setDDEConnectionManager(DDEConnectionManager* ddeMgr);
            void setWindowManager(WindowManager* wndMgr);
            void setSidebarToggleCallback(std::function<void(bool)> cb);
        private:
            Logger& m_logger;
            std::function<void()> m_onExit;
            std::function<void()> m_onAbout;
            ::DDEConnectionManager* m_pDDEClientMgr{nullptr};
            WindowManager* m_pWndMgr{nullptr};
            std::function<void(bool)> m_onSidebarToggle;
    };
}
