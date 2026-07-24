#include <format>

#include "imgui.h"

#include "ui_operation_monitor.h"

namespace gui {
    void UiOperationMonitor::renderGlobalStatusBar() {
        if (!hasActiveTasks()) return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float barHeight = ImGui::GetFrameHeight() * 1.3f;

        ImGui::SetNextWindowPos(ImVec2(0.0f, viewport->Size.y - barHeight));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, barHeight));
        ImGui::Begin("##GlobalStatusBar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoDocking);

        bool first = true;
        for (const auto& [id, t] : getTasks()) {
            if (t.ddeOperationId == 0) continue;
            auto* op = findDdeOp(t.ddeOperationId);
            if (!op) continue;
            if (op->status != ZemaxDDE::OperationStatus::Pending &&
                op->status != ZemaxDDE::OperationStatus::InFlight) continue;

            if (!first) ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 2);
            first = false;

            float progress = op->totalSteps > 0
                ? static_cast<float>(op->currentStep) / op->totalSteps
                : 0.0f;

            ImGui::TextUnformatted(t.label.c_str());
            ImGui::SameLine();
            float barWidth = ImGui::GetContentRegionAvail().x;
            float compactBarHeight = ImGui::GetFrameHeight() * 0.6f;
            ImVec2 barSize(barWidth, compactBarHeight);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ImGui::GetFrameHeight() - compactBarHeight) * 0.5f);
            ImGui::ProgressBar(progress, barSize, "");

            std::string overlay;
            if (!op->message.empty() && op->message.starts_with("Section")) {
                overlay = std::format("{} | DDE {}/{}", op->message, op->currentStep, op->totalSteps);
            } else {
                overlay = std::format("DDE {}/{}", op->currentStep, op->totalSteps);
            }
            float overlayFontScale = 0.75f;
            ImFont* font = ImGui::GetFont();
            float fontSize = ImGui::GetFontSize() * overlayFontScale;
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

        ImGui::End();
    }
}
