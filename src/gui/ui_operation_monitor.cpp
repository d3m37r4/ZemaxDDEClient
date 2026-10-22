#include <chrono>
#include <cmath>
#include <format>
#include <string>
#include <vector>

#include "imgui.h"

#include "ui_operation_monitor.h"

#include "gui/constants.h"
#include "gui/theme_manager.h"
#include "dde/dde_connection_manager.h"
#include "assets/icons/fa/IconsFontAwesome6.h"

namespace gui {
    namespace {
        /// Секунды в «m:ss» (часы добавляются сами, когда появятся).
        std::string formatSeconds(long long totalSec) {
            if (totalSec < 0) totalSec = 0;
            const long long h = totalSec / 3600;
            const long long m = (totalSec / 60) % 60;
            const long long s = totalSec % 60;
            if (h > 0) return std::format("{}:{:02d}:{:02d}", h, m, s);
            return std::format("{}:{:02d}", m, s);
        }

        /// Сколько секунд осталось у выбранной операции: средний темп шагов
        /// (elapsed / currentStep) умножается на число оставшихся шагов.
        /// Недоступно (⇒ -1), пока отметки старта нет или шаг ещё нулевой.
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
        // Постоянная высота: бар больше не появляется/исчезает, поэтому док
        // не дёргается при старте и окончании задач (бэклог п. 17).
        float frameH = ImGui::GetFrameHeight();
        float verticalPadding = ImGuiUtils::DpiScale(STATUS_BAR_VPAD);
        return frameH + 2.0f * verticalPadding;
    }

    void UiOperationMonitor::renderGlobalStatusBar() {
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

        const bool hasTasks = !activeTasks.empty();
        if (m_selectedTaskIndex >= static_cast<int>(activeTasks.size())) {
            m_selectedTaskIndex = 0;
        }

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float barHeight = computeStatusBarHeight();

        ImGui::SetNextWindowPos(ImVec2(0.0f, viewport->Size.y - barHeight));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, barHeight));
        // Тултипы внутри бара наследуют WindowPadding из стека стилей, поэтому
        // падение до нуля обрезает их текст — исходное значение вернём локально.
        const ImVec2 windowPaddingBackup = ImGui::GetStyle().WindowPadding;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("##GlobalStatusBar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoDocking);

        const auto& sem = m_themeManager ? m_themeManager->semantic()
                                         : SemanticPalette{};

        const float vpad = ImGuiUtils::DpiScale(STATUS_BAR_VPAD);
        const float gap = ImGuiUtils::DpiScale(STATUS_BAR_ELEMENT_GAP);
        ImGui::SetCursorPosY(vpad);
        ImGui::SetCursorPosX(gap);

        // ---- Слева: состояние DDE-соединения (индикатор, без кликов) --------
        if (m_connectionManager) {
            const int activeIdx = m_connectionManager->getActiveIndex();
            const bool connectionLost = m_connectionManager->hasConnectionLost();
            const bool connected = (activeIdx >= 0) && !connectionLost;

            std::string connectedText;
            const char* icon;
            const char* value;
            const ImVec4& color = connectionLost ? sem.warning
                                 : connected   ? sem.success
                                               : sem.danger;
            if (connectionLost) {
                icon = ICON_FA_TRIANGLE_EXCLAMATION;
                value = "Connection Lost";
            } else if (connected) {
                icon = ICON_FA_CIRCLE_CHECK;
                connectedText = std::format("DDE: Connected [Slot {}]", activeIdx);
                value = connectedText.c_str();
            } else {
                icon = ICON_FA_CIRCLE_XMARK;
                value = "DDE: Disconnected";
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(icon);
            ImGui::SameLine(0.0f, gap);
            ImGui::TextUnformatted(value);
            ImGui::PopStyleColor();
        }

        if (!hasTasks) {
            ImGui::SameLine(0.0f, gap);
            ImGui::TextColored(sem.muted, "Idle");
            ImGui::PopStyleVar();
            ImGui::End();
            return;
        }

        ImGui::SameLine(0.0f, gap);

        const auto& selected = activeTasks[m_selectedTaskIndex];
        const auto* op = selected.op;
        const auto* record = selected.record;

        // ---- Спиннер --------------------------------------------------------
        ImGuiUtils::BusyArc("##taskbusy", ImGui::GetFrameHeight() * 0.65f,
                             ImGui::GetStyleColorVec4(ImGuiCol_Text), ImGui::GetFrameHeight());
        ImGui::SameLine(0.0f, gap);

        // ---- Комбо выбора задачи --------------------------------------------
        {
            std::string comboPreview = record->label;
            if (record->clientSlot >= 0) {
                comboPreview += std::format(" [Slot {}]", record->clientSlot);
            }

            // Список комбо — отдельное попап-окно: создаясь внутри бара, оно
            // наследует его нулевой WindowPadding и рисует пункты без полей.
            // Строго на время BeginCombo возвращаем нормальный паддинг; сам
            // превью-виджет уже positioned в окне бара, его это не касается.
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

        // ---- Вертикальный разделитель ---------------------------------------
        {
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            float separatorHeight = ImGui::GetFrameHeight() * 0.8f;
            float separatorY = cursorPos.y + (ImGui::GetFrameHeight() - separatorHeight) * 0.5f;
            ImGui::GetWindowDrawList()->AddLineV(
                cursorPos.x, separatorY, separatorY + separatorHeight,
                ImGui::GetColorU32(ImGuiCol_Separator));
        }
        ImGui::SameLine(0.0f, gap);

        // ---- Прогресс с текстом ----------------------------------------------
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
            overlay += std::format(" | ~{} time remaining", formatSeconds(eta));
        }
        // Встроенный overlay ProgressBar в 1.93 центрирует текст по заполненной
        // части ⇒ надпись прижата к краю на 0% и уезжает вправо по ходу расчёта
        // (снято на снимках пользователя). Рисуем по центру ВЕСЬ бара вручную.
        ImGui::ProgressBar(progress, barSize, "");
        ImFont* font = ImGui::GetFont();
        const float fontSize = ImGui::GetFontSize();
        const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, overlay.c_str());
        const ImVec2 barMin = ImGui::GetItemRectMin();
        const ImVec2 barMax = ImGui::GetItemRectMax();
        const ImVec2 textPos(
            barMin.x + (barMax.x - barMin.x - textSize.x) * 0.5f,
            barMin.y + (barMax.y - barMin.y - textSize.y) * 0.5f
        );
        ImGui::GetWindowDrawList()->AddText(font, fontSize, textPos,
            ImGui::GetColorU32(ImGuiCol_Text), overlay.c_str());

        // ---- Cancel только у выбранной задачи (бэклог п. 19) ----------------
        if (canCancel) {
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - cancelWidth - rightMargin);
            ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover,
                                         sem.dangerButtonActive, sem.onAccent);
            if (ImGui::Button(ICON_FA_XMARK "##status_cancel", ImVec2(cancelWidth, 0))) {
                m_onCancelTask(record->source);
            }
            ImGuiUtils::PopFilledButton();
            if (ImGui::IsItemHovered()) {
                // Вернуть нормальный WindowPadding: нулевой из бара обрезает текст тултипа.
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPaddingBackup);
                ImGui::SetTooltip("Cancel this calculation");
                ImGui::PopStyleVar();
            }
        }

        ImGui::PopStyleVar();
        ImGui::End();
    }
}
