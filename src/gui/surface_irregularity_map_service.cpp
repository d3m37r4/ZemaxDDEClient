#include <cmath>
#include <format>
#include "dde/constants.h"
#include "dde/operation_monitor.h"
#include "dde/utils.h"

#include "surface_irregularity_map_service.h"
#include "logger/logger.h"
#include "lib/implot3d/implot3d.h"

namespace {
    constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;
}

namespace gui {
    SurfaceIrregularityMapService::SurfaceIrregularityMapService(DDEConnectionManager* connectionManager, Logger& logger)
        : m_connectionManager(connectionManager)
        , m_logger(logger)
        , m_calculator(connectionManager, logger)
    {
    }

    void SurfaceIrregularityMapService::setUiOperationMonitor(UiOperationMonitor* monitor) {
        m_uiOpMonitor = monitor;
        m_calculator.setMonitor(monitor);
    }

    // --- Profile calculation (via SurfaceProfileCalculator) ---

    void SurfaceIrregularityMapService::startCalculation(int surface, int sampling, double angle, TaskSource source) {
        m_calculator.onComplete = [this]() {
            m_nominalSurfaceData = m_calculator.getResult();
            if (m_nominalSurfaceData.isValid()) {
                m_logger.addLog("[IrregularityMapService] Nominal reference set");
            }
            if (onCalculationComplete) onCalculationComplete();
        };
        m_calculator.onFailed = [this]() {
            if (onCalculationComplete) onCalculationComplete();
        };

        m_calculator.startCalculation(surface, sampling, angle, source);
    }

    void SurfaceIrregularityMapService::cancelCalculation() {
        m_calculator.cancel();
    }

    // --- Map calculation (own logic) ---

    void SurfaceIrregularityMapService::startMapCalculation(int surface, int numRadii, double angleStepDeg) {
        auto* client = getClient();
        if (!client) {
            m_mapError = "No active DDE connection";
            return;
        }

        if (angleStepDeg <= 0.0 || angleStepDeg > 360.0) {
            onMapError("Invalid angle step, must be in range (0, 360]");
            return;
        }

        if (numRadii < 2) {
            onMapError("Invalid number of radii, must be >= 2");
            return;
        }

        int totalPoints = numRadii * static_cast<int>(360.0 / angleStepDeg);
        if (m_calculator.getSource() != TaskSource::None) {
            // Reuse monitor through calculator's mechanism if available
        }

        m_mapError.clear();
        m_targetSurface = surface;
        m_numRadii = numRadii;
        m_angleStepDeg = angleStepDeg;
        m_numAngles = static_cast<int>(360.0 / angleStepDeg);
        m_currentRing = 0;
        m_currentAngle = 0;
        m_skippedPoints = 0;
        m_mapTaskId = 0;
        m_sections.clear();
        m_surfaceRequestsRemaining = 2;
        m_units = client->getOpticalSystemData().units;

        m_logger.addLog(std::format("[IrregularityMapService] Starting surface map: surface {}, radii={}, angle step={} deg ({} total points)",
            surface, numRadii, angleStepDeg, totalPoints));

        client->submitRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::TYPE_NAME),
            [this](const std::string& result) {
                onMapSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::TYPE_NAME, result);
            },
            [this](const std::string& error) {
                onMapError(std::format("GetSurfaceData(TYPE_NAME): {}", error));
            },
            2000, 1, "SurfaceIrregularityMap");

        client->submitRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER),
            [this](const std::string& result) {
                onMapSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER, result);
            },
            [this](const std::string& error) {
                onMapError(std::format("GetSurfaceData(SEMI_DIAMETER): {}", error));
            },
            2000, 1, "SurfaceIrregularityMap");
    }

    void SurfaceIrregularityMapService::cancelMapCalculation() {
        if (m_uiOpMonitor && m_mapTaskId > 0) {
            m_uiOpMonitor->requestCancel(m_mapTaskId);
        }
    }

    void SurfaceIrregularityMapService::onMapSurfaceDataReceived(int code, const std::string& value) {
        auto tokens = ZemaxDDE::tokenize(value);
        if (tokens.empty()) {
            onMapError(std::format("GetSurfaceData({}): empty response", code));
            return;
        }

        if (code == ZemaxDDE::SurfaceDataCode::TYPE_NAME) {
            // type stored per ring, not needed here
        } else if (code == ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER) {
            try {
                m_semiDiameter = std::stod(tokens[0]);
            } catch (...) {
                onMapError("GetSurfaceData(SEMI_DIAMETER): invalid number");
                return;
            }
        }

        if (--m_surfaceRequestsRemaining > 0) return;

        if (m_semiDiameter <= 0.0) {
            onMapError("Surface semi-diameter is zero or negative");
            return;
        }

        m_currentRing = 0;
        m_currentAngle = 0;
        m_currentRingData = {};
        m_currentRingData.id = m_targetSurface;
        m_currentRingData.semiDiameter = m_semiDiameter;
        m_currentRingData.units = m_units;

        int totalPoints = m_numRadii * m_numAngles;
        if (m_uiOpMonitor) {
            m_mapTaskId = m_uiOpMonitor->startTask(TaskSource::SurfaceIrregularityMap, "Surface Irregularity Map", totalPoints);
        }
        sendNextSagPoint();
    }

    void SurfaceIrregularityMapService::sendNextSagPoint() {
        if (m_uiOpMonitor && m_mapTaskId > 0) {
            if (m_uiOpMonitor->isCancelled(m_mapTaskId)) {
                m_mapError = "Cancelled";
                m_uiOpMonitor->failTask(m_mapTaskId, "Cancelled");
                return;
            }
            int step = m_currentRing * m_numAngles + m_currentAngle;
            int total = m_numRadii * m_numAngles;
            if (step < total) {
                m_uiOpMonitor->reportProgress(m_mapTaskId, step, "");
            }
        }

        if (m_currentRing >= m_numRadii) {
            if (m_uiOpMonitor && m_mapTaskId > 0) {
                m_uiOpMonitor->completeTask(m_mapTaskId);
            }
            m_windowState.tolerancedSurfaceIndex = m_targetSurface;
            m_windowState.tolerancedSampling = m_numRadii;
            m_windowState.tolerancedAngleStep = m_angleStepDeg;

            m_logger.addLog(std::format("[IrregularityMapService] Surface map completed: {} rings", m_sections.size()));
            return;
        }

        if (m_currentAngle >= m_numAngles) {
            m_sections.push_back(m_currentRingData);
            m_currentRing++;
            m_currentAngle = 0;
            m_currentRingData = {};
            m_currentRingData.id = m_targetSurface;
            m_currentRingData.semiDiameter = m_semiDiameter;
            m_currentRingData.units = m_units;
            sendNextSagPoint();
            return;
        }

        double radiusStep = m_semiDiameter / (m_numRadii - 1);
        double r = m_currentRing * radiusStep;
        double angle = m_currentAngle * m_angleStepDeg;
        double rad = angle * DEG_TO_RAD;
        double x = r * std::cos(rad);
        double y = r * std::sin(rad);

        auto* client = getClient();
        if (!client) {
            onMapError("Connection lost during map calculation");
            return;
        }

        client->submitRequest(
            std::format("GetSag,{},{},{}", m_targetSurface, x, y),
            [this](const std::string& result) {
                onSagPointReceived(result);
            },
            [this](const std::string& error) {
                if (error == "Timeout") {
                    onSagTimeout();
                } else {
                    onMapError(std::format("GetSag failed: {}", error));
                }
            },
            1000, 1, "SurfaceIrregularityMap");
    }

    void SurfaceIrregularityMapService::onSagPointReceived(const std::string& buffer) {
        auto tokens = ZemaxDDE::tokenize(buffer);
        if (tokens.size() < 2) {
            onMapError("GetSag: invalid response format");
            return;
        }

        double radiusStep = m_semiDiameter / (m_numRadii - 1);
        double r = m_currentRing * radiusStep;
        double angle = m_currentAngle * m_angleStepDeg;
        double rad = angle * DEG_TO_RAD;

        try {
            ZemaxDDE::SagData point;
            point.x = r * std::cos(rad);
            point.y = r * std::sin(rad);
            point.sag = std::stod(tokens[0]);
            point.alternateSag = std::stod(tokens[1]);
            m_currentRingData.sagDataPoints.push_back(point);
        } catch (...) {
            onMapError("GetSag: failed to parse sag values");
            return;
        }

        m_currentAngle++;
        sendNextSagPoint();
    }

    void SurfaceIrregularityMapService::onSagTimeout() {
        m_logger.addLog(std::format("[IrregularityMapService] Point (ring {}, angle {}) timed out, skipping",
            m_currentRing, m_currentAngle));
        m_skippedPoints++;
        m_currentAngle++;
        sendNextSagPoint();
    }

    void SurfaceIrregularityMapService::onMapError(const std::string& error) {
        m_mapError = error;
        if (m_uiOpMonitor && m_mapTaskId > 0) {
            m_uiOpMonitor->failTask(m_mapTaskId, error);
        }
        m_logger.addLog(std::format("[IrregularityMapService] {}", error));
    }

    void SurfaceIrregularityMapService::renderSurfaceMapPlot(const ImVec2& size) {
        if (m_sections.empty()) return;

        int numRadii = static_cast<int>(m_sections.size());
        int numAngles = static_cast<int>(m_sections[0].sagDataPoints.size());
        double semiDiameter = m_sections[0].semiDiameter;
        double angleStepDeg = m_windowState.tolerancedAngleStep;
        double radiusStep = semiDiameter / (numRadii - 1);

        std::vector<float> X(numRadii * numAngles);
        std::vector<float> Y(numRadii * numAngles);
        std::vector<float> Z(numRadii * numAngles);
        float zMin = 0, zMax = 0;
        bool first = true;

        for (int i = 0; i < numRadii; ++i) {
            double r = i * radiusStep;
            for (int j = 0; j < numAngles; ++j) {
                double angle = j * angleStepDeg * DEG_TO_RAD;
                X[i * numAngles + j] = static_cast<float>(r * std::cos(angle));
                Y[i * numAngles + j] = static_cast<float>(r * std::sin(angle));
                const auto& pt = m_sections[i].sagDataPoints[j];
                Z[i * numAngles + j] = static_cast<float>(pt.sag);
                if (first) { zMin = pt.sag; zMax = pt.sag; first = false; }
                else { zMin = std::min(zMin, static_cast<float>(pt.sag)); zMax = std::max(zMax, static_cast<float>(pt.sag)); }
            }
        }

        if (ImPlot3D::BeginPlot("##Surface3D", size)) {
            ImPlot3D::SetupAxes("X (mm)", "Y (mm)", "Z (mm)");
            ImPlot3D::PlotSurface("Lens Surface", X.data(), Y.data(), Z.data(), numRadii, numAngles, zMin, zMax);
            ImPlot3D::EndPlot();
        }
    }
}
