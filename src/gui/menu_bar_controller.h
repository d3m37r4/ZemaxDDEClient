#pragma once

#include <functional>
class Logger;
class DDEConnectionManager;
class DockableWindowsManager;

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
            void setUpdatesCallback(std::function<void()> cb);
            void setDDEConnectionManager(DDEConnectionManager* ddeMgr);
            void setWindowManager(DockableWindowsManager* wndMgr);
            void setSidebarToggleCallback(std::function<void(bool)> cb);
            void setPreferencesCallback(std::function<void()> cb);

            /// Invokes the registered preferences callback (if any). Used by the
            /// application main loop to dispatch the global Ctrl+, shortcut.
            void openPreferences();
        private:
            Logger& m_logger;
            std::function<void()> m_onExit;
            std::function<void()> m_onAbout;
            std::function<void()> m_onUpdates;
            std::function<void()> m_onPreferences;
            ::DDEConnectionManager* m_pDDEClientMgr{nullptr};
            DockableWindowsManager* m_pWndMgr{nullptr};
            std::function<void(bool)> m_onSidebarToggle;
    };
}
