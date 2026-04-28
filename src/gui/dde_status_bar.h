#pragma once

#include <functional>
#include <memory>

class DdeConnectionManager;
class Logger;
class DdeStatusBar;

namespace gui {
  class DdeStatusBar {
  public:
    using Callback = std::function<void()>;
    DdeStatusBar(DdeConnectionManager* dde, Logger& logger);
    void render();
    void setConnectCallback(Callback cb);
    void setDisconnectCallback(Callback cb);
    void updateConnection(bool connected);
  private:
    DdeConnectionManager* m_dde;
    Logger& m_logger;
    Callback m_onConnect;
    Callback m_onDisconnect;
    bool m_connected{false};
  };
}
