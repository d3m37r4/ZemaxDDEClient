#include <cmath>
#include <format>
#include "dde/constants.h"
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

        m_mapState = SagMapState::FetchingSurfaceData;
        m_mapError.clear();
        m_targetSurface = surface;
        m_numRadii = numRadii;
        m_angleStepDeg = angleStepDeg;
        m_numAngles = static_cast<int>(360.0 / angleStepDeg);
        m_currentRing = 0;
        m_currentAngle = 0;
        m_sections.clear();
        m_pendingSurfaceRequests = 2;
        m_units = client->getOpticalSystemData().units;

        m_logger.addLog(std::format("[SagMap] Starting async surface map: surface {}, radii={}, angle step={} deg",
            surface, numRadii, angleStepDeg));

        client->sendRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::TYPE_NAME),
            [this](const std::string& result) {
                onSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::TYPE_NAME, result);
            },
            [this](const std::string& error) {
                onMapError(std::format("GetSurfaceData(TYPE_NAME): {}", error));
            }
        );

        client->sendRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER),
            [this](const std::string& result) {
                onSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER, result);
            },
            [this](const std::string& error) {
                onMapError(std::format("GetSurfaceData(SEMI_DIAMETER): {}", error));
            }
        );
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

        if (--m_pendingSurfaceRequests > 0) return;

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

        client->sendRequest(
            std::format("GetSag,{},{},{}", m_targetSurface, x, y),
            [this](const std::string& result) {
                onSagPointReceived(result);
            },
            [this](const std::string& error) {
                onMapError(std::format("GetSag failed: {}", error));
            }
        );
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

    void SagMapAnalysisService::onMapError(const std::string& error) {
        m_mapState = SagMapState::Failed;
        m_mapError = error;
        m_logger.addLog(std::format("[SagMap] {}", error));
        if (onCalculationComplete) onCalculationComplete();
    }

    bool SagMapAnalysisService::hasNominalReference() const {
        auto* client = getClient();
        return client && client->getNominalSurface()->isValid();
    }

    const ZemaxDDE::SurfaceData& SagMapAnalysisService::getNominalReference() const {
        return *getClient()->getNominalSurface();
    }
}
