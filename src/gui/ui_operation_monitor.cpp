#include <chrono>
#include <cmath>
#include <format>
#include <string>
#include <vector>

#include "imgui.h"

#include "ui_operation_monitor.h"

#include "gui/constants.h"
#include "gui/theme_manager.h"
#include "gui/window_chrome.h"
#include "dde/dde_connection_manager.h"
#include "assets/icons/fa/IconsFontAwesome6.h"

namespace gui {
    namespace {
        /// Seconds to "h:mm:ss" / "m:ss" (hours are appended when they appear).
        std::string formatSeconds(long long totalSec) {
            if (totalSec < 0) totalSec = 0;
            const long long h = totalSec / 3600;
            const long long m = (totalSec / 60) % 60;
            const long long s = totalSec % 60;
            if (h > 0) return std::format("{}:{:02d}:{:02d}", h, m, s);
            return std::format("{}:{:02d}", m, s);
        }

        /// How many seconds the selected operation has left: the average step
        /// pace (elapsed / currentStep) times the number of remaining steps.
        /// Not available (-1) until the start mark exists or the step is zero.
        long long estimateEtaSeconds(const ZemaxDDE::OperationInfo& op,
                                     const app::services::OperationMonitorService::TaskRecord& rec) {
            if (op.currentStep <= 0 || op.currentStep >= op.totalSteps) return -1;
            if (rec.startedAt.time_since_epoch().count() == 0) return -1;
            const double elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - rec.startedAt).count();
            const double perStep = elapsed / static_cast<double>(op.currentStep);
            const double remaining = perStep * static_cast<double>(op.totalSteps - op.currentStep);
            return static_cast<long long>(std::llround(remaining));
        }
    }

    float UiOperationMonitor::computeStatusBarHeight() const {
        // Constant height: the bar no longer appears/disappears, so the dock
        // area does not jump when tasks start or finish. The minimized-window
        // shelf is an OVERLAY on top of floating windows: it does not reserve
        // layout height, so the dock area keeps the same size whether tiles
        // exist or not. The bottom-block height reported to chrome is the bar
        // itself, so a maximized window still ends at the bar top.
        float frameH = ImGui::GetFrameHeight();
        float verticalPadding = ImGuiUtils::DpiScale(STATUS_BAR_VPAD);
        const float bar = frameH + 2.0f * verticalPadding;
        chrome::setBottomBlockHeight(bar);
        return bar;
    }

    void UiOperationMonitor::renderGlobalStatusBar() {
        struct ActiveTask {
            uint64_t id;
            const TaskRecord* record;
            const ZemaxDDE::OperationInfo* op;
        };
        std::vector<ActiveTask> activeTasks;

        for (const auto& [id, t] : getTasks()) {
            if (t.ddeOperationId == 0 || !t.boundMonitor) continue;
            const ZemaxDDE::OperationInfo* foundOp = nullptr;
            for (const auto& op : t.boundMonitor->getOperations()) {
                if (op.id == t.ddeOperationId) {
                    foundOp = &op;
                    break;
                }
            }
            if (!foundOp) continue;
            if (foundOp->status != ZemaxDDE::OperationStatus::Pending &&
                foundOp->status != ZemaxDDE::OperationStatus::InFlight) continue;
            activeTasks.push_back({id, &t, foundOp});
        }

        const bool hasTasks = !activeTasks.empty();
        if (m_selectedTaskIndex >= static_cast<int>(activeTasks.size())) {
            m_selectedTaskIndex = 0;
        }

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        // The bar occupies only its own frame; the minimized-windows shelf
        // (chrome::renderShelf) owns the band between the bar top and the
        // dock area bottom. Sizing the bar to the full bottom block (the
        // computeStatusBarHeight() total) would cover the tiles, because the
        // bar is rendered after the shelf.
        const float barOnly = ImGui::GetFrameHeight()
            + 2.0f * ImGuiUtils::DpiScale(STATUS_BAR_VPAD);

        // Minimized windows shelf: rendered before the bar window so its
        // tiles exist regardless of the early-return branch below. At this
        // point all regular windows already pushed their tiles this frame.
        chrome::renderShelf();

        // Multi-viewport: window coords are desktop coords, so anchor the bar
        // to the main viewport's own position and pin it there (otherwise it
        // drifts to the desktop edge and gets its own platform window).
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x,
            viewport->Pos.y + viewport->Size.y - barOnly));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, barOnly));
        ImGui::SetNextWindowViewport(viewport->ID);
        // Tooltips opened from inside the bar inherit WindowPadding from the
        // style stack; dropping it to zero cuts off their text, so the theme
        // value is restored locally where they are created.
        const ImVec2 windowPaddingBackup = ImGui::GetStyle().WindowPadding;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        // Square corners against the dockspace edge: the theme's windowRounding
        // would round them off even though the bar spans the full width.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("##GlobalStatusBar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoDocking);

        const auto& sem = m_themeManager ? m_themeManager->semantic()
                                         : SemanticPalette{};

        const float vpad = ImGuiUtils::DpiScale(STATUS_BAR_VPAD);
        const float gap = ImGuiUtils::DpiScale(STATUS_BAR_ELEMENT_GAP);
        const float frameH = ImGui::GetFrameHeight();

        // The left segment is drawn through the draw list instead of text
        // widgets: text rows are shorter than frame-height widgets, and
        // SameLine lining would anchor the widget row to the text top. The
        // full-height widgets below start their own row at vpad on all paths.
        const ImVec2 windowPos = ImGui::GetWindowPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImFont* font = ImGui::GetFont();
        const float fontSize = ImGui::GetFontSize();
        // Center the text vertically against the widget row it precedes.
        const float textY = windowPos.y + vpad
            + (frameH - ImGui::GetTextLineHeight()) * 0.5f;
        float nextX = windowPos.x + gap;

        if (m_connectionManager) {
            const int activeIdx = m_connectionManager->getActiveIndex();
            const bool connectionLost = m_connectionManager->hasConnectionLost();
            const bool connected = (activeIdx >= 0) && !connectionLost;

            std::string connectedText;
            const char* value;
            const ImVec4& color = connectionLost ? sem.warning
                                 : connected   ? sem.success
                                               : sem.danger;
            if (connectionLost) {
                value = "Connection Lost";
            } else if (connected) {
                connectedText = std::format("DDE: Connected [Slot {}]", activeIdx);
                value = connectedText.c_str();
            } else {
                value = "DDE: Disconnected";
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
            drawList->AddText(font, fontSize, ImVec2(nextX, textY), textColor, value);
            nextX += font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, value).x;
            ImGui::PopStyleColor();
        }

        if (!hasTasks) {
            // WindowPadding (bar) + WindowRounding pushed before Begin.
            ImGui::PopStyleVar(2);
            ImGui::End();
            return;
        }

        // Widget row starts just past the connection segment (one extra gap
        // only when the segment was actually drawn).
        ImGui::SetCursorScreenPos(ImVec2(
            nextX + (m_connectionManager ? gap : 0.0f), windowPos.y + vpad));

        const auto& selected = activeTasks[m_selectedTaskIndex];
        const auto* op = selected.op;
        const auto* record = selected.record;

        ImGuiUtils::BusyArc("##taskbusy", ImGui::GetFrameHeight() * 0.65f,
                             ImGui::GetStyleColorVec4(ImGuiCol_Text), ImGui::GetFrameHeight());
        ImGui::SameLine(0.0f, gap);

        {
            std::string comboPreview = record->label;
            if (record->clientSlot >= 0) {
                comboPreview += std::format(" [Slot {}]", record->clientSlot);
            }

            // The combo list is a separate popup window: created inside the
            // bar, it inherits the bar's zero WindowPadding and draws its
            // items without insets. Restore the theme padding strictly for
            // the BeginCombo scope; the preview widget is already positioned
            // in the bar window and is not affected.
            float comboWidth = ImGuiUtils::DpiScale(OPERATION_COMBO_BASE_WIDTH);
            ImGui::SetNextItemWidth(comboWidth);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPaddingBackup);
            if (ImGui::BeginCombo("##TaskSelector", comboPreview.c_str())) {
                for (int i = 0; i < static_cast<int>(activeTasks.size()); ++i) {
                    const auto& at = activeTasks[i];
                    std::string itemLabel = at.record->label;
                    if (at.record->clientSlot >= 0) {
                        itemLabel += std::format(" [Slot {}]", at.record->clientSlot);
                    }
                    bool isSelected = (i == m_selectedTaskIndex);
                    if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                        m_selectedTaskIndex = i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopStyleVar();
        }

        ImGui::SameLine(0.0f, gap);

        {
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            float separatorHeight = ImGui::GetFrameHeight() * 0.8f;
            float separatorY = cursorPos.y + (ImGui::GetFrameHeight() - separatorHeight) * 0.5f;
            ImGui::GetWindowDrawList()->AddLineV(
                cursorPos.x, separatorY, separatorY + separatorHeight,
                ImGui::GetColorU32(ImGuiCol_Separator));
        }
        ImGui::SameLine(0.0f, gap);

        float progress = op->totalSteps > 0
            ? static_cast<float>(op->currentStep) / op->totalSteps
            : 0.0f;

        const bool canCancel = static_cast<bool>(m_onCancelTask);
        const float cancelWidth = canCancel ? ImGui::GetFrameHeight() : 0.0f;
        const float rightMargin = ImGuiUtils::DpiScale(STATUS_BAR_ELEMENT_GAP);
        float barWidth = ImGui::GetContentRegionAvail().x - rightMargin
            - (canCancel ? gap + cancelWidth : 0.0f);
        ImVec2 barSize(barWidth, ImGui::GetFrameHeight());

        std::string overlay;
        if (!op->message.empty() && op->message.starts_with("Section")) {
            overlay = std::format("{} | DDE {}/{}", op->message, op->currentStep, op->totalSteps);
        } else {
            overlay = std::format("DDE {}/{}", op->currentStep, op->totalSteps);
        }
        if (const long long eta = estimateEtaSeconds(*op, *record); eta >= 0) {
            overlay += std::format(" | time remaining ~{}", formatSeconds(eta));
        }
        // The built-in ProgressBar overlay of ImGui 1.93 centers its text over
        // the filled part: the caption hugs the edge at 0% and drifts right as
        // the calculation progresses (captured on user screenshots). Draw it
        // centered over the WHOLE bar manually instead.
        ImGui::ProgressBar(progress, barSize, "");
        const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, overlay.c_str());
        const ImVec2 barMin = ImGui::GetItemRectMin();
        const ImVec2 barMax = ImGui::GetItemRectMax();
        const ImVec2 textPos(
            barMin.x + (barMax.x - barMin.x - textSize.x) * 0.5f,
            barMin.y + (barMax.y - barMin.y - textSize.y) * 0.5f
        );
        ImGui::GetWindowDrawList()->AddText(font, fontSize, textPos,
            ImGui::GetColorU32(ImGuiCol_Text), overlay.c_str());

        // Cancel is available for the selected task only.
        if (canCancel) {
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - cancelWidth - rightMargin);
            ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover,
                                         sem.dangerButtonActive, sem.onAccent);
            if (ImGui::Button(ICON_FA_XMARK "##status_cancel", ImVec2(cancelWidth, 0))) {
                m_onCancelTask(record->taskId, record->source);
            }
            ImGuiUtils::PopFilledButton();
            if (ImGui::IsItemHovered()) {
                // Restore the normal WindowPadding: the bar's zero padding
                // would cut off the tooltip text.
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPaddingBackup);
                ImGui::SetTooltip("Cancel this calculation");
                ImGui::PopStyleVar();
            }
        }

        // WindowPadding (bar) + WindowRounding pushed before Begin.
        ImGui::PopStyleVar(2);
        ImGui::End();
    }
}
