#pragma once

#include <functional>
class Logger;
class DdeConnectionManager;
class WindowManager;

namespace gui {
    /**
     * @brief Handles rendering of the top navigation bar.
     *        Stage 2: functionality will be moved from GuiManager.
     */
class MenuBarController {
public:
    explicit MenuBarController(Logger& logger, DdeConnectionManager* ddeMgr);
    void render();
    void setExitCallback(std::function<void()> cb);
    void setAboutCallback(std::function<void()> cb);
    void setDdeConnectionManager(DdeConnectionManager* ddeMgr);
    void setWindowManager(WindowManager* wndMgr);
    void setSidebarToggleCallback(std::function<void(bool)> cb);
private:
    Logger& m_logger;
    std::function<void()> m_onExit;
    std::function<void()> m_onAbout;
    ::DdeConnectionManager* m_pDdeMgr{nullptr};
    WindowManager* m_pWndMgr{nullptr};
    std::function<void(bool)> m_onSidebarToggle;
};
}
