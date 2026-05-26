#include <format>
#include <cmath>

#include "dde/constants.h"
#include "dde/utils.h"

#include "lib/imgui/imgui.h"
#include "lib/implot/implot.h"

#include "gui/surface_profile_service.h"
#include "dde/operation_monitor.h"
#include "gui/gui.h"
#include "logger/logger.h"

namespace gui {
    SurfaceProfileService::SurfaceProfileService(DDEConnectionManager* connectionManager, Logger& logger)
        : m_connectionManager(connectionManager)
        , m_logger(logger)
    {
    }

    std::pair<std::vector<double>, std::vector<double>> extractSagCoordinates(const ZemaxDDE::SurfaceData& surface) {
        std::vector<double> x_vals, y_vals;

        x_vals.reserve(surface.sagDataPoints.size());
        y_vals.reserve(surface.sagDataPoints.size());

        for (const auto& point : surface.sagDataPoints) {
            // Recover radial coordinate r from (x, y) with sign preservation
            // r = sign(x) * sqrt(x^2 + y^2) gives correct range from -semiDiameter to +semiDiameter
            double r = (point.x >= 0 ? 1.0 : -1.0) * std::sqrt(point.x * point.x + point.y * point.y);
            x_vals.push_back(r);
            y_vals.push_back(point.sag);
        }

        return {std::move(x_vals), std::move(y_vals)};
    }

    void SurfaceProfileService::startAsyncSagCalculation(int surface, int sampling, double angle) {
        auto* client = getClient();
        if (!client) {
            m_calcState = SagCalcState::Failed;
            m_calcError = "No active DDE connection";
            if (onCalculationComplete) onCalculationComplete();
            return;
        }

        auto* monitor = getMonitor();
        m_operationId = monitor ? monitor->registerOperation("SurfaceProfile", sampling) : 0;

        m_calcState = SagCalcState::FetchingSurfaceData;
        m_calcError.clear();
        m_targetSurface = surface;
        m_targetSampling = sampling;
        m_targetAngle = angle;
        m_sagPointIndex = 0;
        m_skippedPoints = 0;
        m_calcStartTime = std::chrono::steady_clock::now();
        m_resultSurface = {};
        m_resultSurface.id = surface;
        m_resultSurface.sagDataPoints.clear();
        m_surfaceRequestsRemaining = 2;

        m_logger.addLog(std::format("[SurfaceProfileService] Starting profile calculation for surface {} ({} pts, {}°)", surface, sampling, angle));

        client->submitRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::TYPE_NAME),
            [this](const std::string& result) {
                onSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::TYPE_NAME, result);
            },
            [this](const std::string& error) {
                onError(std::format("GetSurfaceData(TYPE_NAME) failed: {}", error));
            },
            2000, 1, "SurfaceProfile");

        client->submitRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER),
            [this](const std::string& result) {
                onSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER, result);
            },
            [this](const std::string& error) {
                onError(std::format("GetSurfaceData(SEMI_DIAMETER) failed: {}", error));
            },
            2000, 1, "SurfaceProfile");
    }

    void SurfaceProfileService::onSurfaceDataReceived(int code, const std::string& value) {
        auto tokens = ZemaxDDE::tokenize(value);
        if (tokens.empty()) {
            onError(std::format("GetSurfaceData({}): empty response", code));
            return;
        }

        if (code == ZemaxDDE::SurfaceDataCode::TYPE_NAME) {
            m_resultSurface.type = tokens[0];
        } else if (code == ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER) {
            try {
                m_resultSurface.semiDiameter = std::stod(tokens[0]);
            } catch (...) {
                onError("GetSurfaceData(SEMI_DIAMETER): invalid number");
                return;
            }
        }

        if (--m_surfaceRequestsRemaining > 0) return;

        m_calcState = SagCalcState::FetchingSagPoints;
        sendNextSagRequest();
    }

    void SurfaceProfileService::sendNextSagRequest() {
        int totalPoints = 2 * m_targetSampling - 1;
        if (m_sagPointIndex >= totalPoints) {
            m_resultSurface.sampling = totalPoints;
            m_resultSurface.angle = m_targetAngle;
            m_calcState = SagCalcState::Completed;

            auto* monitor = getMonitor();
            if (monitor) {
                auto msg = m_skippedPoints > 0
                    ? std::format("Completed ({} points, {} skipped)", m_resultSurface.sagDataPoints.size(), m_skippedPoints)
                    : std::format("Completed ({} points)", m_resultSurface.sagDataPoints.size());
                monitor->reportProgress(m_operationId, totalPoints, msg);
                monitor->onCompleted(m_operationId);
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_calcStartTime);
            if (m_skippedPoints > 0) {
                m_logger.addLog(std::format("[SurfaceProfileService] Profile completed: {}/{} points ({} skipped) in {}",
                    m_resultSurface.sagDataPoints.size(), totalPoints, m_skippedPoints,
                    ZemaxDDE::formatDuration(elapsed)));
            } else {
                m_logger.addLog(std::format("[SurfaceProfileService] Profile completed: {}/{} points in {}",
                    m_resultSurface.sagDataPoints.size(), totalPoints,
                    ZemaxDDE::formatDuration(elapsed)));
            }
            if (onCalculationComplete) onCalculationComplete();
            return;
        }

        auto* monitor = getMonitor();
        if (monitor && monitor->isCancelled(m_operationId)) {
            m_calcState = SagCalcState::Failed;
            m_calcError = "Cancelled";
            monitor->onError(m_operationId, "Cancelled");
            m_logger.addLog("[SurfaceProfileService] Calculation cancelled by user");
            if (onCalculationComplete) onCalculationComplete();
            return;
        }

        if (monitor) {
            monitor->reportProgress(m_operationId, m_sagPointIndex,
                std::format("Point {}/{}", m_sagPointIndex, totalPoints));
        }

        constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;
        const double rad = m_targetAngle * DEG_TO_RAD;
        double semiDiameter = m_resultSurface.semiDiameter;
        double step = semiDiameter / (m_targetSampling - 1);
        double r = -semiDiameter + m_sagPointIndex * step;
        double x = r * std::cos(rad);
        double y = r * std::sin(rad);

        auto* client = getClient();
        if (!client) {
            onError("Connection lost during profile calculation");
            return;
        }

        client->submitRequest(
            std::format("GetSag,{},{},{}", m_targetSurface, x, y),
            [this](const std::string& result) {
                onSagDataReceived(result);
            },
            [this](const std::string& error) {
                if (error == "Timeout") {
                    onSagTimeout();
                } else {
                    onError(std::format("GetSag failed: {}", error));
                }
            },
            1000, 1, "SurfaceProfile");
    }

    void SurfaceProfileService::onSagDataReceived(const std::string& buffer) {
        auto tokens = ZemaxDDE::tokenize(buffer);
        if (tokens.size() < 2) {
            onError("GetSag: invalid response format");
            return;
        }

        constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;
        const double rad = m_targetAngle * DEG_TO_RAD;
        double semiDiameter = m_resultSurface.semiDiameter;
        double step = semiDiameter / (m_targetSampling - 1);
        double r = -semiDiameter + m_sagPointIndex * step;

        try {
            ZemaxDDE::SagData point;
            point.x = r * std::cos(rad);
            point.y = r * std::sin(rad);
            point.sag = std::stod(tokens[0]);
            point.alternateSag = std::stod(tokens[1]);
            m_resultSurface.sagDataPoints.push_back(point);
        } catch (...) {
            onError("GetSag: failed to parse sag values");
            return;
        }

        m_sagPointIndex++;
        sendNextSagRequest();
    }

    void SurfaceProfileService::onSagTimeout() {
        m_logger.addLog(std::format("[SurfaceProfileService] Point {} timed out, skipping", m_sagPointIndex));
        m_skippedPoints++;
        m_sagPointIndex++;
        sendNextSagRequest();
    }

    void SurfaceProfileService::onError(const std::string& error) {
        m_calcState = SagCalcState::Failed;
        m_calcError = error;

        auto* monitor = getMonitor();
        if (monitor) monitor->onError(m_operationId, error);

        m_logger.addLog(std::format("[SurfaceProfileService] {}", error));
        if (onCalculationComplete) onCalculationComplete();
    }

    void SurfaceProfileService::cancelCalculation() {
        auto* monitor = getMonitor();
        if (monitor && m_operationId > 0) monitor->requestCancel(m_operationId);
    }

    ZemaxDDE::OperationMonitor* SurfaceProfileService::getMonitor() const {
        auto* client = getClient();
        return client ? client->getOperationMonitor() : nullptr;
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

        if (ImPlot::BeginPlot(plotLabel, size)) {
            std::string unitName = getUnitString(surface.units);
            ImPlot::SetupAxes(std::format("X ({})", unitName).c_str(),
                              std::format("Sag ({})", unitName).c_str());
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine("Surface", x_vals.data(), y_vals.data(), x_vals.size());
            ImPlot::EndPlot();
        }
    }

    void SurfaceProfileService::renderProfileComparisonPlot(const char* plotLabel, const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, const ImVec2& size) {
        auto [x_nom, y_nom] = extractSagCoordinates(nominal);
        auto [x_tol, y_tol] = extractSagCoordinates(toleranced);

        if (ImPlot::BeginPlot(plotLabel, size)) {
            ImPlot::SetupAxes("X (mm)", "Sag (mm)");
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine("Nominal", x_nom.data(), y_nom.data(), x_nom.size());
            ImPlot::PlotLine("Toleranced", x_tol.data(), y_tol.data(), x_tol.size());
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

        if (ImPlot::BeginPlot(plotLabel, size)) {
            ImPlot::SetupAxes("X (mm)", "ΔSag (mm)");
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlot::PlotLine("Deviation", x_nom.data(), y_dev.data(), x_nom.size());
            ImPlot::EndPlot();
        }
    }

    ZemaxDDE::ZemaxDDEClient* SurfaceProfileService::getClient() const {
        return m_connectionManager ? m_connectionManager->getActiveClient() : nullptr;
    }
}
