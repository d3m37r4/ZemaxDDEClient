#include "dde/dde_connection_manager.h"
#include "gui/dde_status_bar.h"
#include "gui/gui.h"
#include "logger/logger.h"
#include <imgui.h>

namespace gui {
  DdeStatusBar::DdeStatusBar(DdeConnectionManager* dde, Logger& logger)
    : m_dde(dde), m_logger(logger) {}

  void DdeStatusBar::render() {
    // Check if we should render
    if (!m_dde) return;

    // Use menu bar style
    const float height = 60.0f;
    auto displaySize = ImGui::GetIO().DisplaySize;

    // Style like menu bar
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    ImGui::SetNextWindowPos(ImVec2(0, displaySize.y - height));
    ImGui::SetNextWindowSize(ImVec2(displaySize.x, height));
    ImGui::Begin("DDE Status", nullptr,
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus
    );

    bool connected = m_dde->isConnected();

    // Calculate vertical center position
    const float lineHeight = ImGui::GetFrameHeight();
    const float centerY = (height - lineHeight) * 0.5f;

    // First line: DDE Status (centered)
    ImGui::SetCursorPosY(centerY);
    ImGui::SameLine(8.0f); // Left margin like menu bar
    ImGui::Text("DDE Status:");
    ImGui::SameLine();
    if (connected) {
      ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 255, 100, 255));
      ImGui::Text("● Connected");
      ImGui::PopStyleColor();
    } else {
      ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
      ImGui::Text("○ Disconnected");
      ImGui::PopStyleColor();
    }
    ImGui::SameLine();

    // Second line: Action message (centered)
    if (connected) {
      ImGui::Text("| Server: Zemax");
      ImGui::SameLine();
      // Disconnect button
      ImGui::SameLine();
      if (ImGui::Button("Disconnect")) {
        if (m_onDisconnect) m_onDisconnect();
      }
    } else {
      ImGui::Text("| Click to connect to Zemax");
      ImGui::SameLine();
      // Connect button
      ImGui::SameLine();
      if (ImGui::Button("Connect")) {
        if (m_onConnect) m_onConnect();
      }
    }

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
  }

  void DdeStatusBar::setConnectCallback(Callback cb) {
    m_onConnect = std::move(cb);
  }

  void DdeStatusBar::setDisconnectCallback(Callback cb) {
    m_onDisconnect = std::move(cb);
  }

  void DdeStatusBar::updateConnection(bool connected) {
    m_connected = connected;
  }
}
