#include <format>
#include <cmath>

#include "assets/icons/fa/IconsFontAwesome6.h"
#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"

#include "gui/surface_profile_service.h"
#include "gui/settings_manager.h"
#include "gui/utils.h"
#include "logger/logger.h"

namespace gui {
    namespace {
        ImPlotAxisFlags axisFlagsForGrid(bool showGrid) {
            return showGrid ? ImPlotAxisFlags_None : ImPlotAxisFlags_NoGridLines;
        }

        ImPlotSpec buildLineSpec(float lineWeight, float markerSize) {
            return ImPlotSpec(ImPlotProp_LineWeight, lineWeight,
                              ImPlotProp_MarkerSize, markerSize);
        }

        const char* kDisplayUnitNames[] = { "mm", "μm" };

        int displayUnitCombo(const char* label, app::services::DisplayUnit& unit) {
            int staticUnit = static_cast<int>(unit);
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0f);
            if (ImGui::Combo(label, &staticUnit, kDisplayUnitNames, IM_ARRAYSIZE(kDisplayUnitNames))) {
                unit = static_cast<app::services::DisplayUnit>(staticUnit);
                return 1;
            }
            return 0;
        }
    }

    void SurfaceProfileService::saveCrossSectionToFile(const app::models::SurfaceData& surface) {
        if (surface.sagDataPoints.empty()) {
            m_logger.addLog(std::format("[GUI] No Sag Cross Section data to save for surface {}", surface.id));
            return;
        }

        std::string content;
        content += "Listing of Surface Sag Cross Section Data\n\n";
        content += std::format("Surface {}.\n", surface.id);
        content += std::format("Coordinate units are {}.\n", gui::getUnitString(surface.units, true));
        content += std::format("Units are {}.\n", gui::getUnitString(surface.units, true));
        content += std::format("Width = {} {}.\n", surface.diameter(), gui::getUnitString(surface.units, true));
        content += std::format("Cross section is oriented at an angle of {:.2f} degrees.\n", surface.angle);
        content += std::format("Cross section calculated with {} sampling points.\n\n", surface.sampling);
        content += std::format("{:<15}{:<15}{:<15}\n", "X-Coordinate", "Y-Coordinate", "Sag");

        for (const auto& point : surface.sagDataPoints) {
            content += std::format("{:<15.6e}{:<15.6e}{:<15.6e}\n", point.x, point.y, point.sag);
        }

        auto tempPathOpt = gui::writeToTemporaryFile("ZemaxDDE_SagCrossSection_Temp.txt", content);
        if (!tempPathOpt) {
            m_logger.addLog("[GUI] Failed to create temporary file for Surface Sag Cross Section export");
            return;
        }

        ShellExecuteW(nullptr, L"open", tempPathOpt->c_str(), nullptr, nullptr, SW_SHOW);
        m_logger.addLog(std::format("[GUI] Surface Sag Cross Section saved to {}", tempPathOpt->string()));
    }

    static constexpr const char* kColormapNames[] = { "Cool", "Aqua-Purple", "Ocean", "Aurora" };

    void SurfaceProfileService::renderToolbar(app::services::AxisUnits& units, bool is3D,
                                               int* colormapIdx, bool* showWorst, float* worstColor) {
        if (!ImGui::BeginMenuBar()) return;

        if (ImGui::BeginMenu(ICON_FA_GEARS " Settings")) {
            ImGui::TextUnformatted("X axis");
            ImGui::SameLine(ImGui::GetFontSize() * 6.0f);
            displayUnitCombo("##UnitX", units.x);

            if (is3D) {
                ImGui::TextUnformatted("Y axis");
                ImGui::SameLine(ImGui::GetFontSize() * 6.0f);
                displayUnitCombo("##UnitY", units.y);
            }

            const char* yLabel = is3D ? "Z axis" : "Sag axis";
            ImGui::TextUnformatted(yLabel);
            ImGui::SameLine(ImGui::GetFontSize() * 6.0f);
            displayUnitCombo(is3D ? "##UnitZ" : "##UnitY", is3D ? units.z : units.y);

            if (is3D && showWorst && worstColor) {
                ImGui::Separator();
                ImGui::Checkbox("##ShowWorst", showWorst);
                ImGui::SameLine();
                ImGui::TextUnformatted("Show worst");
                ImGui::SameLine();
                ImGui::ColorEdit3("##WorstColor", worstColor,
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            }

            ImGui::EndMenu();
        }

        if (is3D && colormapIdx) {
            if (ImGui::BeginMenu(ICON_FA_PALETTE " Colormap")) {
                for (int i = 0; i < IM_ARRAYSIZE(kColormapNames); ++i) {
                    if (ImGui::MenuItem(kColormapNames[i], nullptr, *colormapIdx == i)) {
                        *colormapIdx = i;
                    }
                }
                ImGui::EndMenu();
            }
        }

        ImGui::EndMenuBar();
    }

    void SurfaceProfileService::renderSurfaceProfilePlot(const char* plotLabel, const app::models::SurfaceData& surface, const ImVec2& size) {
        if (surface.sagDataPoints.empty()) return;

        auto [x_vals, y_vals] = app::services::extractSagCoordinates(surface);
        const bool showGrid = m_settingsManager ? m_settingsManager->showGridByDefault() : true;
        const float lineWeight = m_settingsManager ? m_settingsManager->plotLineWeight() : 1.0f;
        const float markerSize = m_settingsManager ? m_settingsManager->plotMarkerSize() : 4.0f;

        const app::services::AxisUnits& units = m_windowState.units;
        double scaleX = gui::unitScaleFactor(units.x);
        double scaleY = gui::unitScaleFactor(units.y);

        std::vector<double> x_scaled(x_vals.size()), y_scaled(y_vals.size());
        for (size_t i = 0; i < x_vals.size(); ++i) {
            x_scaled[i] = x_vals[i] * scaleX;
            y_scaled[i] = y_vals[i] * scaleY;
        }

        if (ImPlot::BeginPlot(plotLabel, size)) {
            const char* unitX = gui::displayUnitName(units.x);
            const char* unitY = gui::displayUnitName(units.y);
            ImPlot::SetupAxes(std::format("X ({})", unitX).c_str(),
                              std::format("Sag ({})", unitY).c_str(),
                              axisFlagsForGrid(showGrid), axisFlagsForGrid(showGrid));
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine("Surface", x_scaled.data(), y_scaled.data(), x_scaled.size(),
                             buildLineSpec(lineWeight, markerSize));
            ImPlot::EndPlot();
        }
    }

    void SurfaceProfileService::renderProfileComparisonPlot(const char* plotLabel, const app::models::SurfaceData& nominal, const app::models::SurfaceData& toleranced, const ImVec2& size) {
        auto [x_nom, y_nom] = app::services::extractSagCoordinates(nominal);
        auto [x_tol, y_tol] = app::services::extractSagCoordinates(toleranced);
        const bool showGrid = m_settingsManager ? m_settingsManager->showGridByDefault() : true;
        const float lineWeight = m_settingsManager ? m_settingsManager->plotLineWeight() : 1.0f;
        const float markerSize = m_settingsManager ? m_settingsManager->plotMarkerSize() : 4.0f;

        const app::services::AxisUnits& units = m_windowState.units;
        double scaleX = gui::unitScaleFactor(units.x);
        double scaleY = gui::unitScaleFactor(units.y);

        std::vector<double> x_nom_s(x_nom.size()), y_nom_s(y_nom.size());
        std::vector<double> x_tol_s(x_tol.size()), y_tol_s(y_tol.size());
        for (size_t i = 0; i < x_nom.size(); ++i) {
            x_nom_s[i] = x_nom[i] * scaleX;
            y_nom_s[i] = y_nom[i] * scaleY;
        }
        for (size_t i = 0; i < x_tol.size(); ++i) {
            x_tol_s[i] = x_tol[i] * scaleX;
            y_tol_s[i] = y_tol[i] * scaleY;
        }

        if (ImPlot::BeginPlot(plotLabel, size)) {
            const char* unitX = gui::displayUnitName(units.x);
            const char* unitY = gui::displayUnitName(units.y);
            ImPlot::SetupAxes(std::format("X ({})", unitX).c_str(),
                              std::format("Sag ({})", unitY).c_str(),
                              axisFlagsForGrid(showGrid), axisFlagsForGrid(showGrid));
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine("Nominal", x_nom_s.data(), y_nom_s.data(), x_nom_s.size(),
                             buildLineSpec(lineWeight, markerSize));
            ImPlot::PlotLine("Toleranced", x_tol_s.data(), y_tol_s.data(), x_tol_s.size(),
                             buildLineSpec(lineWeight, markerSize));
            ImPlot::EndPlot();
        }
    }

    void SurfaceProfileService::renderProfileDeviationPlot(const char* plotLabel, const app::models::SurfaceData& nominal, const app::models::SurfaceData& toleranced, const ImVec2& size) {
        auto [x_nom, y_nom] = app::services::extractSagCoordinates(nominal);
        auto [x_tol, y_tol] = app::services::extractSagCoordinates(toleranced);

        if (x_nom.size() != x_tol.size()) return;

        std::vector<double> y_dev;
        y_dev.reserve(x_nom.size());
        for (size_t i = 0; i < x_nom.size(); ++i)
            y_dev.push_back(y_tol[i] - y_nom[i]);

        const bool showGrid = m_settingsManager ? m_settingsManager->showGridByDefault() : true;
        const float lineWeight = m_settingsManager ? m_settingsManager->plotLineWeight() : 1.0f;
        const float markerSize = m_settingsManager ? m_settingsManager->plotMarkerSize() : 4.0f;

        const app::services::AxisUnits& units = m_windowState.units;
        double scaleX = gui::unitScaleFactor(units.x);
        double scaleY = gui::unitScaleFactor(units.y);

        std::vector<double> x_s(x_nom.size()), y_s(y_dev.size());
        for (size_t i = 0; i < x_nom.size(); ++i) {
            x_s[i] = x_nom[i] * scaleX;
            y_s[i] = y_dev[i] * scaleY;
        }

        if (ImPlot::BeginPlot(plotLabel, size)) {
            const char* unitX = gui::displayUnitName(units.x);
            const char* unitY = gui::displayUnitName(units.y);
            ImPlot::SetupAxes(std::format("X ({})", unitX).c_str(),
                              std::format("ΔSag ({})", unitY).c_str(),
                              axisFlagsForGrid(showGrid), axisFlagsForGrid(showGrid));
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine("Deviation", x_s.data(), y_s.data(), x_s.size(),
                             buildLineSpec(lineWeight, markerSize));
            ImPlot::EndPlot();
        }
    }
}
