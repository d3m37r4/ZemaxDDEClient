#include <format>

#include "imgui.h"

#include "ui_operation_monitor.h"

namespace gui {

uint64_t UiOperationMonitor::startTask(TaskSource source, const std::string& label, int totalSteps) {
    uint64_t ddeId = 0;
    if (m_monitor) {
        ddeId = m_monitor->registerOperation(label, totalSteps);
    }

    TaskRecord rec;
    rec.taskId = m_nextId++;
    rec.source = source;
    rec.label = label;
    rec.ddeOperationId = ddeId;
    m_tasks.push_back(rec);

    return rec.taskId;
}

void UiOperationMonitor::reportProgress(uint64_t taskId, int currentStep, const std::string& message) {
    auto* rec = findRecord(taskId);
    if (!rec || !m_monitor) return;
    m_monitor->reportProgress(rec->ddeOperationId, currentStep, message);
}

bool UiOperationMonitor::isCancelled(uint64_t taskId) const {
    auto* rec = findRecord(taskId);
    if (!rec || !m_monitor) return false;
    return m_monitor->isCancelled(rec->ddeOperationId);
}

void UiOperationMonitor::completeTask(uint64_t taskId) {
    auto* rec = findRecord(taskId);
    if (!rec || !m_monitor) return;
    m_monitor->onCompleted(rec->ddeOperationId);
    rec->ddeOperationId = 0;
}

void UiOperationMonitor::failTask(uint64_t taskId, const std::string& error) {
    auto* rec = findRecord(taskId);
    if (!rec || !m_monitor) return;
    m_monitor->onError(rec->ddeOperationId, error);
    rec->ddeOperationId = 0;
}

void UiOperationMonitor::requestCancel(uint64_t taskId) {
    auto* rec = findRecord(taskId);
    if (!rec) return;
    if (m_monitor && rec->ddeOperationId > 0)
        m_monitor->requestCancel(rec->ddeOperationId);
}

uint64_t UiOperationMonitor::getDdeOperationId(uint64_t taskId) const {
    auto* rec = findRecord(taskId);
    return rec ? rec->ddeOperationId : 0;
}

bool UiOperationMonitor::isActive(TaskSource source) const {
    for (const auto& t : m_tasks) {
        if (t.source != source) continue;
        if (t.ddeOperationId == 0) continue;
        auto* op = findDdeOp(t.ddeOperationId);
        if (!op) continue;
        if (op->status == ZemaxDDE::OperationStatus::Pending ||
            op->status == ZemaxDDE::OperationStatus::InFlight) {
            return true;
        }
    }
    return false;
}

bool UiOperationMonitor::hasActiveTasks() const {
    for (const auto& t : m_tasks) {
        if (t.ddeOperationId == 0) continue;
        auto* op = findDdeOp(t.ddeOperationId);
        if (!op) continue;
        if (op->status == ZemaxDDE::OperationStatus::Pending ||
            op->status == ZemaxDDE::OperationStatus::InFlight) {
            return true;
        }
    }
    return false;
}

void UiOperationMonitor::renderGlobalStatusBar() {
    if (!hasActiveTasks() || !m_monitor) return;

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
    for (const auto& t : m_tasks) {
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

        std::string overlay = std::format("{}/{}", op->currentStep, op->totalSteps);
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

UiOperationMonitor::TaskRecord* UiOperationMonitor::findRecord(uint64_t taskId) {
    for (auto& t : m_tasks) {
        if (t.taskId == taskId) return &t;
    }
    return nullptr;
}

const UiOperationMonitor::TaskRecord* UiOperationMonitor::findRecord(uint64_t taskId) const {
    for (const auto& t : m_tasks) {
        if (t.taskId == taskId) return &t;
    }
    return nullptr;
}

const ZemaxDDE::OperationInfo* UiOperationMonitor::findDdeOp(uint64_t ddeId) const {
    if (!m_monitor) return nullptr;
    for (const auto& op : m_monitor->getOperations()) {
        if (op.id == ddeId) return &op;
    }
    return nullptr;
}

}
