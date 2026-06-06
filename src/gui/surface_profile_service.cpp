#include <format>
#include <cmath>

#include "dde/constants.h"
#include "dde/utils.h"

#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"

#include "gui/settings_manager.h"
#include "gui/surface_profile_service.h"
#include "gui/gui.h"
#include "logger/logger.h"

namespace gui {
    SurfaceProfileService::SurfaceProfileService(DDEConnectionManager* connectionManager, Logger& logger)
        : m_connectionManager(connectionManager)
        , m_logger(logger)
        , m_calculator(connectionManager, logger)
    {
    }

    void SurfaceProfileService::setUiOperationMonitor(UiOperationMonitor* monitor) {
        m_calculator.setMonitor(monitor);
    }

    namespace {
        ImPlotAxisFlags axisFlagsForGrid(bool showGrid) {
            return showGrid ? ImPlotAxisFlags_None : ImPlotAxisFlags_NoGridLines;
        }

        ImPlotSpec buildLineSpec(float lineWeight, float markerSize) {
            return ImPlotSpec(ImPlotProp_LineWeight, lineWeight,
                              ImPlotProp_MarkerSize, markerSize);
        }
    }

    std::pair<std::vector<double>, std::vector<double>> extractSagCoordinates(const ZemaxDDE::SurfaceData& surface) {
        std::vector<double> x_vals, y_vals;

        x_vals.reserve(surface.sagDataPoints.size());
        y_vals.reserve(surface.sagDataPoints.size());

        for (const auto& point : surface.sagDataPoints) {
            double r = (point.x >= 0 ? 1.0 : -1.0) * std::sqrt(point.x * point.x + point.y * point.y);
            x_vals.push_back(r);
            y_vals.push_back(point.sag);
        }

        return {std::move(x_vals), std::move(y_vals)};
    }

    void SurfaceProfileService::startCalculation(int surface, int sampling, double angle, TaskSource source) {
        m_taskSource = source;

        m_calculator.onComplete = [this]() { onCalculatorComplete(); };
        m_calculator.onFailed = [this]() { onCalculatorFailed(); };

        m_calculator.startCalculation(surface, sampling, angle, source);
    }

    void SurfaceProfileService::onCalculatorComplete() {
        const auto& result = m_calculator.getResult();

        if (m_taskSource == TaskSource::NominalSurfaceProfile) {
            m_nominalSurfaceData = result;
        } else {
            m_tolerancedSurfaceData = result;
        }

        if (onCalculationComplete) onCalculationComplete();
    }

    void SurfaceProfileService::onCalculatorFailed() {
        if (onCalculationComplete) onCalculationComplete();
    }

    void SurfaceProfileService::cancelCalculation() {
        m_calculator.cancel();
    }

    const ZemaxDDE::SurfaceData& SurfaceProfileService::getResult() const {
        return m_calculator.getResult();
    }

    void SurfaceProfileService::saveCrossSectionToFile(const ZemaxDDE::SurfaceData& surface) {
        if (surface.sagDataPoints.empty()) {
            m_logger.addLog(std::format("[GUI] No Sag Cross Section data to save for surface {}", surface.id));
            return;
        }

        std::string content;
        content += "Listing of Surface Sag Cross Section Data\n\n";
        content += std::format("Surface {}.\n", surface.id);
        content += std::format("Coordinate units are {}.\n", getUnitString(surface.units, true));
        content += std::format("Units are {}.\n", getUnitString(surface.units, true));
        content += std::format("Width = {} {}.\n", surface.diameter(), getUnitString(surface.units, true));
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

    void SurfaceProfileService::renderSurfaceProfilePlot(const char* plotLabel, const ZemaxDDE::SurfaceData& surface, const ImVec2& size) {
        if (surface.sagDataPoints.empty()) return;

        auto [x_vals, y_vals] = extractSagCoordinates(surface);
        const bool showGrid = m_settingsManager ? m_settingsManager->showGridByDefault() : true;
        const float lineWeight = m_settingsManager ? m_settingsManager->plotLineWeight() : 1.0f;
        const float markerSize = m_settingsManager ? m_settingsManager->plotMarkerSize() : 4.0f;

        if (ImPlot::BeginPlot(plotLabel, size)) {
            std::string unitName = getUnitString(surface.units);
            ImPlot::SetupAxes(std::format("X ({})", unitName).c_str(),
                              std::format("Sag ({})", unitName).c_str(),
                              axisFlagsForGrid(showGrid), axisFlagsForGrid(showGrid));
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine("Surface", x_vals.data(), y_vals.data(), x_vals.size(),
                             buildLineSpec(lineWeight, markerSize));
            ImPlot::EndPlot();
        }
    }

    void SurfaceProfileService::renderProfileComparisonPlot(const char* plotLabel, const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, const ImVec2& size) {
        auto [x_nom, y_nom] = extractSagCoordinates(nominal);
        auto [x_tol, y_tol] = extractSagCoordinates(toleranced);
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

    void SurfaceProfileService::renderProfileDeviationPlot(const char* plotLabel, const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, const ImVec2& size) {
        auto [x_nom, y_nom] = extractSagCoordinates(nominal);
        auto [x_tol, y_tol] = extractSagCoordinates(toleranced);

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
