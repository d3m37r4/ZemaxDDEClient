#include <format>
#include <cmath>

#include "dde/constants.h"
#include "dde/utils.h"

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

    void SurfaceProfileService::renderSurfaceProfilePlot(const char* plotLabel, const app::models::SurfaceData& surface, const ImVec2& size) {
        if (surface.sagDataPoints.empty()) return;

        auto [x_vals, y_vals] = app::services::extractSagCoordinates(surface);
        const bool showGrid = m_settingsManager ? m_settingsManager->showGridByDefault() : true;
        const float lineWeight = m_settingsManager ? m_settingsManager->plotLineWeight() : 1.0f;
        const float markerSize = m_settingsManager ? m_settingsManager->plotMarkerSize() : 4.0f;

        if (ImPlot::BeginPlot(plotLabel, size)) {
            std::string unitName = gui::getUnitString(surface.units);
            ImPlot::SetupAxes(std::format("X ({})", unitName).c_str(),
                              std::format("Sag ({})", unitName).c_str(),
                              axisFlagsForGrid(showGrid), axisFlagsForGrid(showGrid));
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine("Surface", x_vals.data(), y_vals.data(), x_vals.size(),
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

        if (ImPlot::BeginPlot(plotLabel, size)) {
            ImPlot::SetupAxes("X (mm)", "Sag (mm)",
                              axisFlagsForGrid(showGrid), axisFlagsForGrid(showGrid));
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine("Nominal", x_nom.data(), y_nom.data(), x_nom.size(),
                             buildLineSpec(lineWeight, markerSize));
            ImPlot::PlotLine("Toleranced", x_tol.data(), y_tol.data(), x_tol.size(),
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

        if (ImPlot::BeginPlot(plotLabel, size)) {
            ImPlot::SetupAxes("X (mm)", "ΔSag (mm)",
                              axisFlagsForGrid(showGrid), axisFlagsForGrid(showGrid));
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine("Deviation", x_nom.data(), y_dev.data(), x_nom.size(),
                             buildLineSpec(lineWeight, markerSize));
            ImPlot::EndPlot();
        }
    }
}
