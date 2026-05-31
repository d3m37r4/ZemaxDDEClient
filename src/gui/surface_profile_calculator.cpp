#include <format>
#include <cmath>
#include <numbers>

#include "surface_profile_calculator.h"
#include "dde/constants.h"
#include "dde/operation_monitor.h"
#include "dde/utils.h"
#include "logger/logger.h"

namespace gui {

    SurfaceProfileCalculator::SurfaceProfileCalculator(
        DDEConnectionManager* connectionManager, Logger& logger)
        : m_connectionManager(connectionManager)
        , m_logger(logger)
    {
    }

    ZemaxDDE::ZemaxDDEClient* SurfaceProfileCalculator::getClient() const {
        return m_connectionManager ? m_connectionManager->getActiveClient() : nullptr;
    }

    bool SurfaceProfileCalculator::isCancelled() const {
        return m_uiOpMonitor && m_taskId > 0 && m_uiOpMonitor->isCancelled(m_taskId);
    }

    void SurfaceProfileCalculator::startCalculation(
        int surface, int sampling, double angle, TaskSource source, const std::string& label)
    {
        auto* client = getClient();
        if (!client) {
            m_state = State::Failed;
            m_error = "No active DDE connection";
            if (onFailed) onFailed();
            return;
        }

        m_source = source;
        if (m_uiOpMonitor) {
            std::string taskLabel = label.empty()
                ? (source == TaskSource::NominalSurfaceProfile ? "Nominal Profile" : "Toleranced Profile")
                : label;
            m_taskId = m_uiOpMonitor->startTask(source, taskLabel, sampling);
        }

        m_state = State::FetchingSurfaceData;
        m_error.clear();
        m_targetSurface = surface;
        m_targetSampling = sampling;
        m_targetAngle = angle;
        m_sagPointIndex = 0;
        m_skippedPoints = 0;
        m_calcStartTime = std::chrono::steady_clock::now();
        m_result = {};
        m_result.id = surface;
        m_result.sagDataPoints.clear();
        m_surfaceRequestsRemaining = 2;

        m_logger.addLog(std::format("[ProfileCalculator] Starting: surface {} ({} pts, {}°)",
            surface, sampling, angle));

        client->submitRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::TYPE_NAME),
            [this](const std::string& result) {
                onSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::TYPE_NAME, result);
            },
            [this](const std::string& error) {
                onError(std::format("GetSurfaceData(TYPE_NAME): {}", error));
            },
            2000, 1, "ProfileCalculator");

        client->submitRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER),
            [this](const std::string& result) {
                onSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER, result);
            },
            [this](const std::string& error) {
                onError(std::format("GetSurfaceData(SEMI_DIAMETER): {}", error));
            },
            2000, 1, "ProfileCalculator");
    }

    void SurfaceProfileCalculator::cancel() {
        if (m_uiOpMonitor && m_taskId > 0) {
            m_uiOpMonitor->requestCancel(m_taskId);
        }
    }

    void SurfaceProfileCalculator::onSurfaceDataReceived(
        int code, const std::string& value)
    {
        auto tokens = ZemaxDDE::tokenize(value);
        if (tokens.empty()) {
            onError(std::format("GetSurfaceData({}): empty response", code));
            return;
        }

        if (code == ZemaxDDE::SurfaceDataCode::TYPE_NAME) {
            m_result.type = tokens[0];
        } else if (code == ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER) {
            try {
                m_result.semiDiameter = std::stod(tokens[0]);
            } catch (...) {
                onError("GetSurfaceData(SEMI_DIAMETER): invalid number");
                return;
            }
        }

        if (--m_surfaceRequestsRemaining > 0) return;

        m_state = State::FetchingSagPoints;
        sendNextSagRequest();
    }

    void SurfaceProfileCalculator::sendNextSagRequest() {
        if (m_sagPointIndex >= m_targetSampling) {
            m_result.sampling = m_targetSampling;
            m_result.angle = m_targetAngle;
            m_state = State::Completed;

            if (m_uiOpMonitor) {
                m_uiOpMonitor->completeTask(m_taskId);
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_calcStartTime);
            if (m_skippedPoints > 0) {
                m_logger.addLog(std::format("[ProfileCalculator] Completed: {}/{} points ({} skipped) in {}",
                    m_result.sagDataPoints.size(), m_targetSampling, m_skippedPoints,
                    ZemaxDDE::formatDuration(elapsed)));
            } else {
                m_logger.addLog(std::format("[ProfileCalculator] Completed: {}/{} points in {}",
                    m_result.sagDataPoints.size(), m_targetSampling,
                    ZemaxDDE::formatDuration(elapsed)));
            }
            if (onComplete) onComplete();
            return;
        }

        if (isCancelled()) {
            m_state = State::Failed;
            m_error = "Cancelled";
            if (m_uiOpMonitor) {
                m_uiOpMonitor->failTask(m_taskId, "Cancelled");
            }
            m_logger.addLog("[ProfileCalculator] Cancelled by user");
            if (onFailed) onFailed();
            return;
        }

        if (m_uiOpMonitor) {
            m_uiOpMonitor->reportProgress(m_taskId, m_sagPointIndex,
                std::format("Point {}/{}", m_sagPointIndex, m_targetSampling));
        }

        constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;
        const double rad = m_targetAngle * DEG_TO_RAD;
        double semiDiameter = m_result.semiDiameter;
        double step = 2.0 * semiDiameter / (m_targetSampling - 1);
        double r = -semiDiameter + m_sagPointIndex * step;
        double x = r * std::cos(rad);
        double y = r * std::sin(rad);

        auto* client = getClient();
        if (!client) {
            onError("Connection lost during calculation");
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
            1000, 1, "ProfileCalculator");
    }

    void SurfaceProfileCalculator::onSagDataReceived(const std::string& buffer) {
        auto tokens = ZemaxDDE::tokenize(buffer);
        if (tokens.size() < 2) {
            onError("GetSag: invalid response format");
            return;
        }

        constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;
        const double rad = m_targetAngle * DEG_TO_RAD;
        double semiDiameter = m_result.semiDiameter;
        double step = 2.0 * semiDiameter / (m_targetSampling - 1);
        double r = -semiDiameter + m_sagPointIndex * step;

        try {
            ZemaxDDE::SagData point;
            point.x = r * std::cos(rad);
            point.y = r * std::sin(rad);
            point.sag = std::stod(tokens[0]);
            point.alternateSag = std::stod(tokens[1]);
            m_result.sagDataPoints.push_back(point);
        } catch (...) {
            onError("GetSag: failed to parse sag values");
            return;
        }

        m_sagPointIndex++;
        sendNextSagRequest();
    }

    void SurfaceProfileCalculator::onSagTimeout() {
        m_logger.addLog(std::format("[ProfileCalculator] Point {} timed out, skipping",
            m_sagPointIndex));
        m_skippedPoints++;
        m_sagPointIndex++;
        sendNextSagRequest();
    }

    void SurfaceProfileCalculator::onError(const std::string& error) {
        m_state = State::Failed;
        m_error = error;

        if (m_uiOpMonitor) {
            m_uiOpMonitor->failTask(m_taskId, error);
        }

        m_logger.addLog(std::format("[ProfileCalculator] {}", error));
        if (onFailed) onFailed();
    }

}
