#pragma once

// Shared calculation form: fields, validation, progress banner and result
// buttons.
//
// The form appears in four instances (nominal and toleranced in the Surface
// Profile Inspector, and in the Surface Irregularity Map). All four are
// shaped alike and differ only in data, so labels, limits, steps and hint
// texts live here and the panels pass values only.
//
// The widget order inside the form matters: it defines the layout
// (Text/SameLine/Input), so the functions below are copied verbatim.

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

/// Tooltip for the Sampling field (identical for all four forms).
inline const char* SamplingTooltip() {
    static const std::string text = std::format(
        "Total number of sample points across the full surface diameter (min={}, max={}).\n"
        "Automatically rounded up to the nearest odd number to ensure a center point at r=0.\n"
        "Higher values = smoother profile, slower calculation.",
        app::compute::MIN_SAG_SAMPLING, app::compute::MAX_SAG_SAMPLING);
    return text.c_str();
}

/// Tooltip for the Angle field.
inline const char* AngleTooltip() {
    return "Orientation angle in degrees relative to the local x axis.";
}

/// Tooltip for the irregularity map Angle step field.
inline const char* AngleStepTooltip() {
    return "Angular step between cross-section profiles in degrees.\n"
           "Profiles span 180° (each profile gives two opposite radial lines).\n"
           "Smaller step = more sections, smoother 3D surface, slower calculation.";
}

/// Data of one calculation form: what to show and what to do.
struct CalcFormContext {
    app::models::TaskSource source = app::models::TaskSource::NominalSurfaceProfile;
    const char*            bannerLabel = nullptr;   // "Nominal Profile"
    std::string            opticalSystem;            // readonly text and copy source
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
    /// true means the angle field is the irregularity map step (0.01-180),
    /// otherwise it is the profile angle (-+360).
    bool                   angleIsStep = false;

    /// Surface count limit for clamping. Negative means the surface count is
    /// not clamped at all (map without a DDE client), zero clamps it to 0
    /// (inspector without a DDE client).
    int                    maxSurfaces = 0;
    bool                   frozen = false;         // form frozen on another slot
    bool                   onActiveSlot = false;   // calculation runs on the active slot
    const char*            actionLabel = nullptr;  // "Get nominal surface data"
    const char*            actionIcon = nullptr;   // ICON_FA_DOWNLOAD
};

/// Right-side hint (?) after the section title: shown while the form is
/// frozen on another slot.
inline void RenderFrozenHint(int calcSlot, int activeSlot) {
    if (calcSlot < 0 || calcSlot == activeSlot) return;
    ImGui::SameLine();
    ImGuiUtils::HelpMarker(
        "Frozen form: values were captured from the slot where the calculation started. "
        "Switch target (F6) to edit, or Cancel the running calculation.");
}

/// Calculation progress banner.
inline void RenderCalcBanner(app::models::TaskSource source, const char* label,
                             gui::UiOperationMonitor& monitor, const ImVec4& color) {
    auto progress = monitor.getTaskProgress(source);
    if (!progress) return;
    int pct = progress->totalSteps > 0 ? (progress->currentStep * 100 / progress->totalSteps) : 0;
    ImGui::TextColored(color, "%s — calculation in progress on slot %d: %d%%", label, progress->clientSlot, pct);
}

/// Progress banner built from the form data.
inline void RenderCalcBanner(const CalcFormContext& ctx, gui::UiOperationMonitor& monitor,
                             const ImVec4& color) {
    RenderCalcBanner(ctx.source, ctx.bannerLabel, monitor, color);
}

/// "Surface number:" field - integer, limits 0..maxSurfaces.
/// @param clampNow  Whether to clamp: the surface count is clamped only
///                  while the form is editable and the limit is known.
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
        // The hint travels with the clamp: for the surface count it is shown
        // only while the form is editable.
        ImGuiUtils::AdjustedHint(flash,
            std::format("Adjusted to 0-{}", ctx.maxSurfaces).c_str(), warning);
    }
}

/// "Sampling:" field - integer, odd values only, MIN/MAX_SAG_SAMPLING limits.
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

/// Angle field: "Angle" (-+360) for profiles, "Angle step" (0.001-180) for the map.
inline void RenderAngleField(const CalcFormContext& ctx, const ImVec4& danger, const ImVec4& warning) {
    const bool isStep = ctx.angleIsStep;
    const double min = isStep ? app::compute::MIN_ANGLE_STEP_DEG : gui::MIN_ANGLE;
    const double max = isStep ? app::compute::MAX_ANGLE_STEP_DEG : gui::MAX_ANGLE;
    ImGui::TextUnformatted(isStep ? "Angle step:" : "Angle:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
    const bool flash = ImGuiUtils::AdjustedFlashActive(*ctx.angleAdjustedAt);
    const bool tint = ImGuiUtils::BeginAdjustedHighlight(flash, danger);
    // The map step is entered with thousandths of a degree, the profile angle
    // with hundredths.
    ImGui::InputDouble(ctx.angleId, ctx.angleValue,
                       isStep ? 0.01 : 1.0, isStep ? 0.1 : 10.0,
                       isStep ? "%.3f" : "%.2f");
    if (tint) ImGui::PopStyleColor();
    if (app::ClampWithFeedback(*ctx.angleValue, min, max)) {
        *ctx.angleAdjustedAt = ImGui::GetTime();
    }
    ImGui::SameLine();
    ImGuiUtils::HelpMarker(isStep ? AngleStepTooltip() : AngleTooltip());
    // Hint strings preserved in their original spacing.
    const std::string hint = isStep
        ? std::format("Adjusted to {}-{}", min, max)
        : std::format("Adjusted to {} - {}", min, max);
    ImGuiUtils::AdjustedHint(flash, hint.c_str(), warning);
}

/// Block "Optical system / Surface number / Sampling / Angle".
inline void RenderCalcForm(const CalcFormContext& ctx, const ImVec4& danger, const ImVec4& warning) {
    // Pad like a property-grid row (CellPadding.y): the form opens with an input
    // frame at the cursor, while a grid row insets its text by CellPadding.y, and
    // without this the first field sits twice as close to the section bar as the
    // text-only property grids do.
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().CellPadding.y));

    {
        // The path always exists while a file is open; empty means no DDE client.
        std::string display = ctx.opticalSystem.empty() ? "Not connected" : ctx.opticalSystem;
        ImGui::TextUnformatted("Optical system:");
        ImGui::SameLine();
        // Fixed width (as in Preferences -> Files): a long path is clipped
        // inside the field and can be scrolled with the cursor to the end.
        // In a narrow panel the width adapts so the copy button stays inside
        // the window.
        // Vertical scrollbar allowance: ImGui draws it over the edge of the
        // content area, and without the reserve the copy button hides under it.
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
        // The full path is copied, even if it is clipped in the field.
        ImGuiUtils::CopyButton(copyLabel.c_str(), ctx.opticalSystem.c_str());
    }

    // The surface count is clamped only while the form is editable and the
    // limit is known; Sampling and the angle are clamped always.
    const bool editable = !ctx.frozen && !ctx.onActiveSlot;
    RenderSurfaceField(ctx, danger, warning, editable && ctx.maxSurfaces >= 0);
    RenderSamplingField(ctx, danger, warning);
    RenderAngleField(ctx, danger, warning);
}

/// Result buttons: "Processing..." + Cancel, or the primary action button.
/// @param onStart   Start the calculation (service-specific).
/// @param onCancel  Cancel the calculation (service-specific).
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
