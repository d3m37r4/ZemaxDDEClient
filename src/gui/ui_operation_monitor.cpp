#include <format>
#include <vector>

#include "imgui.h"
#include "gui/imgui_utils.h"

#include "ui_operation_monitor.h"

namespace gui {
    float UiOperationMonitor::computeStatusBarHeight() const {
        if (!hasActiveTasks()) return 0.0f;
        float frameH = ImGui::GetFrameHeight();
        float verticalPadding = ImGuiUtils::DpiScale(4.0f);
        return frameH + 2.0f * verticalPadding;
    }

    void UiOperationMonitor::renderGlobalStatusBar() {
        if (!hasActiveTasks()) return;

        // Collect active tasks
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

        if (activeTasks.empty()) return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float barHeight = computeStatusBarHeight();

        ImGui::SetNextWindowPos(ImVec2(0.0f, viewport->Size.y - barHeight));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, barHeight));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("##GlobalStatusBar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoDocking);

        // Center content vertically
        float frameH = ImGui::GetFrameHeight();
        float horizontalPadding = ImGuiUtils::DpiScale(4.0f);
        ImGui::SetCursorPosY((barHeight - frameH) * 0.5f);
        ImGui::SetCursorPosX(horizontalPadding);

        // Clamp selected index
        if (m_selectedTaskIndex >= static_cast<int>(activeTasks.size())) {
            m_selectedTaskIndex = 0;
        }

        const auto& selected = activeTasks[m_selectedTaskIndex];
        const auto* op = selected.op;

        // Always show combo (even for single task)
        {
            std::string comboPreview;
            {
                const auto& sel = activeTasks[m_selectedTaskIndex];
                comboPreview = sel.record->label;
                if (sel.record->clientSlot >= 0) {
                    comboPreview += std::format(" [Slot {}]", sel.record->clientSlot);
                }
            }

            float comboWidth = ImGuiUtils::DpiScale(220.0f);
            ImGui::SetNextItemWidth(comboWidth);
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

            ImGui::SameLine(0.0f, ImGuiUtils::DpiScale(4.0f));

            // Vertical separator
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            float separatorHeight = ImGui::GetFrameHeight() * 0.8f;
            float separatorY = cursorPos.y + (ImGui::GetFrameHeight() - separatorHeight) * 0.5f;
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(cursorPos.x, separatorY),
                ImVec2(cursorPos.x, separatorY + separatorHeight),
                ImGui::GetColorU32(ImGuiCol_Separator));
            ImGui::SameLine(0.0f, ImGuiUtils::DpiScale(4.0f));

            // Progress bar for selected task
            float progress = op->totalSteps > 0
                ? static_cast<float>(op->currentStep) / op->totalSteps
                : 0.0f;

            float barWidth = ImGui::GetContentRegionAvail().x - horizontalPadding;
            ImVec2 barSize(barWidth, ImGui::GetFrameHeight());
            ImGui::ProgressBar(progress, barSize, "");

            // Overlay text
            std::string overlay;
            if (!op->message.empty() && op->message.starts_with("Section")) {
                overlay = std::format("{} | DDE {}/{}", op->message, op->currentStep, op->totalSteps);
            } else {
                overlay = std::format("DDE {}/{}", op->currentStep, op->totalSteps);
            }
            float textScale = 1.0f;
            ImFont* font = ImGui::GetFont();
            float fontSize = ImGui::GetFontSize() * textScale;
            ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, overlay.c_str());
            ImVec2 barMin = ImGui::GetItemRectMin();
            ImVec2 barMax = ImGui::GetItemRectMax();
            ImVec2 textPos(
                barMin.x + (barMax.x - barMin.x - textSize.x) * 0.5f,
                barMin.y + (barMax.y - barMin.y - textSize.y) * 0.5f
            );
            ImGui::GetWindowDrawList()->AddText(font, fontSize, textPos,
                ImGui::GetColorU32(ImGuiCol_Text), overlay.c_str());
        }

        ImGui::PopStyleVar();
        ImGui::End();
    }
}
