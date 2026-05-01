#pragma once

#include <functional>
// Forward declaration for Logger type to avoid coupling here
class Logger;
class DdeConnectionManager;

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
private:
    Logger& m_logger;
    std::function<void()> m_onExit;
    std::function<void()> m_onAbout;
    ::DdeConnectionManager* m_pDdeMgr{nullptr};
};
}
