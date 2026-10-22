#pragma once

// Общая форма расчёта: поля, валидация, баннер прогресса и кнопки результата.
//
// Форма расчёта встречается в четырёх экземплярах (nominal и toleranced в
// Surface Profile Inspector и Surface Irregularity Map). Все четыре формы
// устроены одинаково и различаются лишь данными, поэтому подписи, пределы,
// шаги и тексты подсказок живут здесь, а панели передают только значения.
//
// Порядок виджетов внутри формы важен: он определяет раскладку
// (Text/SameLine/Input) и потому переносится без изменений.

#include <algorithm>
#include <format>
#include <functional>
#include <string>

#include "app/compute/surface_map.h"
#include "app/models/types.h"
#include "app/utils.h"
#include "gui/constants.h"
#include "gui/imgui_utils.h"
#include "gui/theme_manager.h"
#include "gui/ui_operation_monitor.h"
#include "assets/icons/fa/IconsFontAwesome6.h"
#include "lib/imgui/imgui.h"

namespace gui::forms {

/// Подсказка к полю Sampling (одинакова для всех четырёх форм).
inline const char* SamplingTooltip() {
    static const std::string text = std::format(
        "Total number of sample points across the full surface diameter (min={}, max={}).\n"
        "Automatically rounded up to the nearest odd number to ensure a center point at r=0.\n"
        "Higher values = smoother profile, slower calculation.",
        app::compute::MIN_SAG_SAMPLING, app::compute::MAX_SAG_SAMPLING);
    return text.c_str();
}

/// Подсказка к полю Angle.
inline const char* AngleTooltip() {
    return "Orientation angle in degrees relative to the local x axis.";
}

/// Подсказка к полю Angle step карты неравномерностей.
inline const char* AngleStepTooltip() {
    return "Angular step between cross-section profiles in degrees.\n"
           "Profiles span 180° (each profile gives two opposite radial lines).\n"
           "Smaller step = more sections, smoother 3D surface, slower calculation.";
}

/// Данные одной формы расчёта: что показать и что делать.
struct CalcFormContext {
    app::models::TaskSource source = app::models::TaskSource::NominalSurfaceProfile;
    const char*            bannerLabel = nullptr;   // "Nominal Profile"
    std::string            opticalSystem;            // текст readonly-поля и копии
    const char*            copyId = nullptr;         // "##copy_optical_nominal"

    const char*            surfaceId = nullptr;      // "##nominal_surf_num"
    int*                   surfaceValue = nullptr;
    double*                surfaceAdjustedAt = nullptr;

    const char*            samplingId = nullptr;     // "##nominal_sampling"
    int*                   samplingValue = nullptr;
    double*                samplingAdjustedAt = nullptr;

    const char*            angleId = nullptr;        // "##nominal_angle"
    double*                angleValue = nullptr;
    double*                angleAdjustedAt = nullptr;
    /// true ⇒ поле угла — шаг карты неравномерностей (0.01–180), иначе — угол
    /// профиля (±360).
    bool                   angleIsStep = false;

    /// Число поверхностей для клампа. Отрицательное ⇒ число поверхностей не
    /// клампится вовсе (карта без DDE-клиента), ноль ⇒ клампится в 0
    /// (инспектор без DDE-клиента).
    int                    maxSurfaces = 0;
    bool                   frozen = false;         // форма заморожена на другом слоте
    bool                   onActiveSlot = false;   // расчёт идёт на активном слоте
    const char*            actionLabel = nullptr;  // "Get nominal surface data"
    const char*            actionIcon = nullptr;   // ICON_FA_DOWNLOAD
};

/// Правая подсказка (?) после заголовка секции: показывается, пока форма
/// заморожена на другом слоте.
inline void RenderFrozenHint(int calcSlot, int activeSlot) {
    if (calcSlot < 0 || calcSlot == activeSlot) return;
    ImGui::SameLine();
    ImGuiUtils::HelpMarker(
        "Frozen form: values were captured from the slot where the calculation started. "
        "Switch target (F6) to edit, or Cancel the running calculation.");
}

/// Баннер прогресса расчёта.
inline void RenderCalcBanner(app::models::TaskSource source, const char* label,
                             gui::UiOperationMonitor& monitor, const ImVec4& color) {
    auto progress = monitor.getTaskProgress(source);
    if (!progress) return;
    int pct = progress->totalSteps > 0 ? (progress->currentStep * 100 / progress->totalSteps) : 0;
    ImGui::TextColored(color, "%s — calculation in progress on slot %d: %d%%", label, progress->clientSlot, pct);
}

/// Баннер прогресса по данным формы.
inline void RenderCalcBanner(const CalcFormContext& ctx, gui::UiOperationMonitor& monitor,
                             const ImVec4& color) {
    RenderCalcBanner(ctx.source, ctx.bannerLabel, monitor, color);
}

/// Поле «Surface number:» — целое, пределы 0..maxSurfaces.
/// @param clampNow  Выполнять ли кламп: число поверхностей клампится, только
///                  пока форма редактируема и предел известен.
inline void RenderSurfaceField(const CalcFormContext& ctx, const ImVec4& danger, const ImVec4& warning,
                                bool clampNow) {
    ImGui::TextUnformatted("Surface number:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
    const bool flash = ImGuiUtils::AdjustedFlashActive(*ctx.surfaceAdjustedAt);
    const bool tint = ImGuiUtils::BeginAdjustedHighlight(flash, danger);
    ImGui::InputInt(ctx.surfaceId, ctx.surfaceValue, 1, 10);
    if (tint) ImGui::PopStyleColor();
    if (clampNow) {
        if (app::ClampWithFeedback(*ctx.surfaceValue, 0, ctx.maxSurfaces)) {
            *ctx.surfaceAdjustedAt = ImGui::GetTime();
        }
        // Подсказка идёт вместе с клампом: у числа поверхностей она
        // показывается только пока форма редактируема.
        ImGuiUtils::AdjustedHint(flash,
            std::format("Adjusted to 0-{}", ctx.maxSurfaces).c_str(), warning);
    }
}

/// Поле «Sampling:» — целое, только нечётные значения, пределы MIN/MAX_SAG_SAMPLING.
inline void RenderSamplingField(const CalcFormContext& ctx, const ImVec4& danger, const ImVec4& warning) {
    ImGui::TextUnformatted("Sampling:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
    const bool flash = ImGuiUtils::AdjustedFlashActive(*ctx.samplingAdjustedAt);
    const bool tint = ImGuiUtils::BeginAdjustedHighlight(flash, danger);
    ImGui::InputInt(ctx.samplingId, ctx.samplingValue, 10, 50);
    if (tint) ImGui::PopStyleColor();
    if (app::ClampWithFeedback(*ctx.samplingValue,
                              app::compute::MIN_SAG_SAMPLING, app::compute::MAX_SAG_SAMPLING)) {
        *ctx.samplingAdjustedAt = ImGui::GetTime();
    }
    if (app::SnapOddWithFeedback(*ctx.samplingValue)) {
        *ctx.samplingAdjustedAt = ImGui::GetTime();
    }
    app::ClampWithFeedback(*ctx.samplingValue,
                           app::compute::MIN_SAG_SAMPLING, app::compute::MAX_SAG_SAMPLING);
    ImGui::SameLine();
    ImGuiUtils::HelpMarker(SamplingTooltip());
    ImGuiUtils::AdjustedHint(flash,
        std::format("Adjusted to {}-{} (odd)",
            app::compute::MIN_SAG_SAMPLING, app::compute::MAX_SAG_SAMPLING).c_str(), warning);
}

/// Поле угла: «Angle» (±360) для профилей, «Angle step» (0.001–180) для карты.
inline void RenderAngleField(const CalcFormContext& ctx, const ImVec4& danger, const ImVec4& warning) {
    const bool isStep = ctx.angleIsStep;
    const double min = isStep ? app::compute::MIN_ANGLE_STEP_DEG : gui::MIN_ANGLE;
    const double max = isStep ? app::compute::MAX_ANGLE_STEP_DEG : gui::MAX_ANGLE;
    ImGui::TextUnformatted(isStep ? "Angle step:" : "Angle:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
    const bool flash = ImGuiUtils::AdjustedFlashActive(*ctx.angleAdjustedAt);
    const bool tint = ImGuiUtils::BeginAdjustedHighlight(flash, danger);
    // Шаг карты вводится с тысячными долями градуса, угол профиля — сотыми.
    ImGui::InputDouble(ctx.angleId, ctx.angleValue,
                       isStep ? 0.01 : 1.0, isStep ? 0.1 : 10.0,
                       isStep ? "%.3f" : "%.2f");
    if (tint) ImGui::PopStyleColor();
    if (app::ClampWithFeedback(*ctx.angleValue, min, max)) {
        *ctx.angleAdjustedAt = ImGui::GetTime();
    }
    ImGui::SameLine();
    ImGuiUtils::HelpMarker(isStep ? AngleStepTooltip() : AngleTooltip());
    // Тексты подсказок сохранены в исходном виде.
    const std::string hint = isStep
        ? std::format("Adjusted to {}-{}", min, max)
        : std::format("Adjusted to {} - {}", min, max);
    ImGuiUtils::AdjustedHint(flash, hint.c_str(), warning);
}

/// Блок «Optical system / Surface number / Sampling / Angle».
inline void RenderCalcForm(const CalcFormContext& ctx, const ImVec4& danger, const ImVec4& warning) {
    {
        // Путь есть всегда, пока открыт файл; пусто — значит нет DDE-клиента.
        std::string display = ctx.opticalSystem.empty() ? "Not connected" : ctx.opticalSystem;
        ImGui::TextUnformatted("Optical system:");
        ImGui::SameLine();
        // Фиксированная ширина (как в Preferences → Files): длинный путь
        // обрезается внутри поля, его можно прокрутить курсором до конца.
        // В узкой панели ширина подстраивается, чтобы кнопка копирования не
        // уехала за пределы окна.
        // Запас под вертикальный скроллбар: ImGui рисует его поверх края
        // контент-области, и без резерва кнопка копирования под ним прячется.
        const float avail = std::max(ImGui::GetContentRegionAvail().x
            - ImGuiUtils::CopyButtonWidth() - ImGui::GetStyle().ItemSpacing.x
            - ImGui::GetStyle().ScrollbarSize, 0.0f);
        ImGui::SetNextItemWidth(std::min(ImGui::GetFontSize() * 32.0f, avail));
        ImGui::InputText("##optical_system", display.data(), display.capacity() + 1,
                         ImGuiInputTextFlags_ReadOnly);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", display.c_str());
        }
        ImGui::SameLine();
        const std::string copyLabel = std::string{ICON_FA_COPY} + " " + ctx.copyId;
        // Копируется полный путь, даже если в поле он обрезан.
        ImGuiUtils::CopyButton(copyLabel.c_str(), ctx.opticalSystem.c_str());
    }

    // Число поверхностей клампится, только пока форма редактируема и предел
    // известен; Sampling и угол клампятся всегда.
    const bool editable = !ctx.frozen && !ctx.onActiveSlot;
    RenderSurfaceField(ctx, danger, warning, editable && ctx.maxSurfaces >= 0);
    RenderSamplingField(ctx, danger, warning);
    RenderAngleField(ctx, danger, warning);
}

/// Кнопки результата: «Processing…» + Cancel, либо основная кнопка действия.
/// @param onStart   Запуск расчёта (сервисно-специфично).
/// @param onCancel  Отмена расчёта (сервисно-специфично).
inline void RenderResultActions(const CalcFormContext& ctx, gui::UiOperationMonitor& monitor,
                                const SemanticPalette& sem, bool ddeInitialized, int activeSlot,
                                std::function<void()> onStart, std::function<void()> onCancel) {
    const std::string actionLabel = std::string{ctx.actionIcon} + " " + ctx.actionLabel;
    if (ctx.frozen || ctx.onActiveSlot) {
        ImGuiUtils::SpinnerButton("Processing...", true);
        ImGui::SameLine();
        ImGuiUtils::PushFilledButton(sem.dangerButton, sem.dangerButtonHover, sem.dangerButtonActive, sem.onAccent);
        if (ImGui::Button(ICON_FA_XMARK " Cancel")) {
            onCancel();
        }
        ImGuiUtils::PopFilledButton();
    } else if (monitor.hasActiveTasksOnSlot(activeSlot)) {
        ImGui::BeginDisabled(true);
        ImGuiUtils::PushFilledButton(sem.successButton, sem.successButtonHover, sem.successButtonActive, sem.onAccent);
        ImGui::Button(actionLabel.c_str());
        ImGuiUtils::PopFilledButton();
        if (ImGui::BeginItemTooltip()) {
            ImGui::TextUnformatted("Another calculation is in progress on this slot");
            ImGui::EndTooltip();
        }
        ImGui::EndDisabled();
    } else {
        ImGui::BeginDisabled(!ddeInitialized);
        ImGuiUtils::PushFilledButton(sem.successButton, sem.successButtonHover, sem.successButtonActive, sem.onAccent);
        if (ImGui::Button(actionLabel.c_str())) {
            onStart();
        }
        ImGuiUtils::PopFilledButton();
        ImGui::EndDisabled();
    }
}

} // namespace gui::forms
