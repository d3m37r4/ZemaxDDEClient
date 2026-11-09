#include <format>

#include "connect_dde.h"
#include "app/zemax_window_enumerator.h"
#include "dde/utils.h"
#include "gui/utils.h"
#include "gui/imgui_utils.h"
#include "gui/notifications.h"
#include "gui/constants.h"
#include "gui/theme_manager.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "lib/imgui/imgui.h"
#include "logger/logger.h"

namespace gui {
    void ConnectDDEPopup::open() noexcept {
        m_open = true;
    }

    void ConnectDDEPopup::close() noexcept {
        m_open = false;
    }

    void ConnectDDEPopup::render() {
        if (!m_open) return;
        if (!m_logger) return;

        if (!ImGui::IsPopupOpen(CONNECT_DDE_POPUP_NAME)) {
            ImGui::OpenPopup(CONNECT_DDE_POPUP_NAME);
        }

        ImGuiUtils::CenterNextWindow();
        ImGuiUtils::SetDpiScaledWindowConstraints(CONNECT_DDE_POPUP_MIN_SIZE.x, CONNECT_DDE_POPUP_MIN_SIZE.y);
        ImGuiUtils::SetDpiScaledWindowSize(CONNECT_DDE_POPUP_DEFAULT_SIZE);

        if (!ImGuiUtils::BeginPopupModalEx(CONNECT_DDE_POPUP_NAME, &m_open, ImGuiWindowFlags_NoCollapse)) {
            return;
        }

        auto windows = app::ZemaxWindowEnumerator::enumerate();

        // Matches an enumerated window against the connected slots so the row
        // can show the "Connected" mark and clicks can switch instead of
        // creating a duplicate connection.
        auto connectedSlotFor = [this](HWND hwnd) -> int {
            for (int slot = 0; slot < m_connectionManager->getMaxConnections(); ++slot) {
                const DDEConnection* conn = m_connectionManager->getConnection(slot);
                if (conn && conn->isConnected() && conn->hwndServer == hwnd) {
                    return slot;
                }
            }
            return -1;
        };

        auto activate = [this, &connectedSlotFor](const app::ZemaxWindowInfo& wnd) {
            const int connectedSlot = connectedSlotFor(wnd.hwnd);
            if (connectedSlot >= 0) {
                m_connectionManager->setActiveConnection(connectedSlot);
                m_logger->addLog(std::format("[DDE] Activated connected window on slot {}", connectedSlot));
                notify::info(std::format("Switched to slot {}", connectedSlot));
                m_open = false;
                m_selectedHwnd = nullptr;
                return;
            }
            const int slot = m_connectionManager->connectToZemax(wnd.hwnd, wnd.title);
            if (slot >= 0) {
                m_open = false;
                m_selectedHwnd = nullptr;
            } else {
                const std::string title = ZemaxDDE::wstring_to_utf8(wnd.title);
                m_logger->addLog(std::format("[DDE] Failed to connect to: {}", title));
                notify::error(std::format("Failed to connect to \"{}\"", title));
            }
        };

        bool selectionAlive = false;

        ImGui::BeginChild("ZemaxWindowList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);

        const ImGuiStyle& style = ImGui::GetStyle();
        const float textLineHeight = ImGui::GetTextLineHeight();

        for (size_t i = 0; i < windows.size(); ++i) {
            const auto& wnd = windows[i];
            if (wnd.hwnd == m_selectedHwnd) {
                selectionAlive = true;
            }
            const int connectedSlot = connectedSlotFor(wnd.hwnd);
            const std::string label = std::format("[PID: {}] {}", wnd.pid,
                                                 ZemaxDDE::wstring_to_utf8(wnd.title));

            // One item for the whole row: the highlight covers the icon too.
            // The icon and text are drawn over the Selectable so the check can
            // keep its green color and its own tooltip area.
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable("##row", wnd.hwnd == m_selectedHwnd,
                                  ImGuiSelectableFlags_AllowDoubleClick)) {
                m_selectedHwnd = wnd.hwnd;
            }

            const ImVec2 rowMin = ImGui::GetItemRectMin();
            const ImVec2 rowMax = ImGui::GetItemRectMax();
            const float textY = rowMin.y + (rowMax.y - rowMin.y - textLineHeight) * 0.5f;
            float textX = rowMin.x + style.FramePadding.x;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            bool iconHovered = false;
            if (connectedSlot >= 0) {
                drawList->AddText(ImVec2(textX, textY),
                                  ImGui::GetColorU32(m_themeManager->semantic().success),
                                  ICON_FA_CIRCLE_CHECK);
                const float iconWidth = ImGui::CalcTextSize(ICON_FA_CIRCLE_CHECK).x;
                iconHovered = ImGui::IsMouseHoveringRect(ImVec2(textX, rowMin.y),
                                                         ImVec2(textX + iconWidth, rowMax.y));
                textX += iconWidth + style.ItemInnerSpacing.x;
            }
            drawList->PushClipRect(rowMin, rowMax, true);
            drawList->AddText(ImVec2(textX, textY), ImGui::GetColorU32(ImGuiCol_Text),
                              label.c_str());
            drawList->PopClipRect();

            if (iconHovered && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Connected");
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                m_selectedHwnd = wnd.hwnd;
                activate(wnd);
            }
            ImGui::PopID();
        }

        if (windows.empty()) {
            ImGui::TextDisabled("No Zemax windows found");
            ImGui::TextDisabled("Start OpticStudio to make it available here.");
        }

        ImGui::EndChild();

        // The selected window may have disappeared since the last frame: keep a
        // single remaining window selected, otherwise drop the selection.
        if (!selectionAlive) {
            m_selectedHwnd = windows.size() == 1 ? windows[0].hwnd : nullptr;
        }

        const app::ZemaxWindowInfo* selected = nullptr;
        for (const auto& wnd : windows) {
            if (wnd.hwnd == m_selectedHwnd) {
                selected = &wnd;
                break;
            }
        }
        const bool canConnect = selected != nullptr;

        float connectBtnW = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);
        float cancelBtnW  = ImGuiUtils::DpiScale(BASE_POPUP_BUTTON_WIDTH);

        // Buttons centered like the other modal dialogs.
        const float buttonsWidth = connectBtnW + cancelBtnW + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonsWidth) * 0.5f);

        if (!canConnect) {
            ImGui::BeginDisabled();
        }

        const auto& sem = m_themeManager->semantic();
        ImGuiUtils::PushFilledButton(sem.successButton, sem.successButtonHover, sem.successButtonActive, sem.onAccent);

        if (ImGui::Button(ICON_FA_PLUG " Connect", ImVec2(connectBtnW, 0))) {
            activate(*selected);
        }

        ImGuiUtils::PopFilledButton();

        if (!canConnect) {
            ImGui::EndDisabled();
        }

        if (canConnect && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            activate(*selected);
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_XMARK " Cancel", ImVec2(cancelBtnW, 0))) {
            m_open = false;
            m_selectedHwnd = nullptr;
        }

        ImGui::EndPopup();
    }
}
