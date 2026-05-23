#include <cmath>
#include <format>
#include "dde/constants.h"
#include "dde/operation_monitor.h"
#include "dde/utils.h"

#include "sag_map_analysis_service.h"
#include "logger/logger.h"

namespace {
    constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;
}

namespace gui {
    SagMapAnalysisService::SagMapAnalysisService(DDEConnectionManager* connectionManager, Logger& logger)
        : m_connectionManager(connectionManager)
        , m_logger(logger)
    {
    }

    void SagMapAnalysisService::startAsyncMapCalculation(int surface, int numRadii, double angleStepDeg) {
        auto* client = getClient();
        if (!client) {
            m_mapState = SagMapState::Failed;
            m_mapError = "No active DDE connection";
            if (onCalculationComplete) onCalculationComplete();
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
        auto* monitor = getMonitor();
        m_operationId = monitor ? monitor->registerOperation("SagMapAnalysis", totalPoints) : 0;

        m_mapState = SagMapState::FetchingSurfaceData;
        m_mapError.clear();
        m_targetSurface = surface;
        m_numRadii = numRadii;
        m_angleStepDeg = angleStepDeg;
        m_numAngles = static_cast<int>(360.0 / angleStepDeg);
        m_currentRing = 0;
        m_currentAngle = 0;
        m_skippedPoints = 0;
        m_sections.clear();
        m_surfaceRequestsRemaining = 2;
        m_units = client->getOpticalSystemData().units;

        m_logger.addLog(std::format("[SagMap] Starting async surface map: surface {}, radii={}, angle step={} deg ({} total points)",
            surface, numRadii, angleStepDeg, totalPoints));

        client->enqueueRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::TYPE_NAME),
            [this](const std::string& result) {
                onSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::TYPE_NAME, result);
            },
            [this](const std::string& error) {
                onMapError(std::format("GetSurfaceData(TYPE_NAME): {}", error));
            },
            2000, 1, "SagMapAnalysis");

        client->enqueueRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER),
            [this](const std::string& result) {
                onSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER, result);
            },
            [this](const std::string& error) {
                onMapError(std::format("GetSurfaceData(SEMI_DIAMETER): {}", error));
            },
            2000, 1, "SagMapAnalysis");
    }

    void SagMapAnalysisService::onSurfaceDataReceived(int code, const std::string& value) {
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

        m_mapState = SagMapState::FetchingSagPoints;
        m_currentRing = 0;
        m_currentAngle = 0;
        m_currentRingData = {};
        m_currentRingData.id = m_targetSurface;
        m_currentRingData.semiDiameter = m_semiDiameter;
        m_currentRingData.units = m_units;
        sendNextSagPoint();
    }

    void SagMapAnalysisService::sendNextSagPoint() {
        if (m_currentRing >= m_numRadii) {
            m_mapState = SagMapState::Completed;
            m_state.tolerancedSurfaceIndex = m_targetSurface;
            m_state.tolerancedSampling = m_numRadii;
            m_state.tolerancedAngleStep = m_angleStepDeg;

            auto* monitor = getMonitor();
            if (monitor) {
                auto msg = m_skippedPoints > 0
                    ? std::format("Completed ({} rings, {} skipped)", m_sections.size(), m_skippedPoints)
                    : std::format("Completed ({} rings)", m_sections.size());
                monitor->reportProgress(m_operationId, m_numRadii * m_numAngles, msg);
                monitor->onCompleted(m_operationId);
            }

            m_logger.addLog(std::format("[SagMap] Surface map completed: {} rings", m_sections.size()));
            if (onCalculationComplete) onCalculationComplete();
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

        auto* monitor = getMonitor();
        if (monitor && monitor->isCancelled(m_operationId)) {
            m_mapState = SagMapState::Failed;
            m_mapError = "Cancelled";
            monitor->onError(m_operationId, "Cancelled");
            m_logger.addLog("[SagMap] Map calculation cancelled by user");
            if (onCalculationComplete) onCalculationComplete();
            return;
        }

        if (monitor) {
            int currentPoint = m_currentRing * m_numAngles + m_currentAngle;
            monitor->reportProgress(m_operationId, currentPoint,
                std::format("Ring {}/{} angle {}/{}", m_currentRing + 1, m_numRadii, m_currentAngle, m_numAngles));
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

        client->enqueueRequest(
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
            1000, 1, "SagMapAnalysis");
    }

    void SagMapAnalysisService::onSagPointReceived(const std::string& buffer) {
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

    void SagMapAnalysisService::onSagTimeout() {
        m_logger.addLog(std::format("[SagMap] Point (ring {}, angle {}) timed out, skipping",
            m_currentRing, m_currentAngle));
        m_skippedPoints++;
        m_currentAngle++;
        sendNextSagPoint();
    }

    void SagMapAnalysisService::onMapError(const std::string& error) {
        m_mapState = SagMapState::Failed;
        m_mapError = error;

        auto* monitor = getMonitor();
        if (monitor) monitor->onError(m_operationId, error);

        m_logger.addLog(std::format("[SagMap] {}", error));
        if (onCalculationComplete) onCalculationComplete();
    }

    void SagMapAnalysisService::cancelCalculation() {
        auto* monitor = getMonitor();
        if (monitor && m_operationId > 0) monitor->requestCancel(m_operationId);
    }

    ZemaxDDE::OperationMonitor* SagMapAnalysisService::getMonitor() const {
        auto* client = getClient();
        return client ? client->getOperationMonitor() : nullptr;
    }

    bool SagMapAnalysisService::hasNominalReference() const {
        auto* client = getClient();
        return client && client->getNominalSurface()->isValid();
    }

    const ZemaxDDE::SurfaceData& SagMapAnalysisService::getNominalReference() const {
        return *getClient()->getNominalSurface();
    }
}
