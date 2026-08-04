#include <format>
#include <cmath>

#include "surface_profile.h"
#include "app/models/surface_data.h"
#include "app/services/operation_monitor_service.h"
#include "dde/constants.h"
#include "dde/operation_monitor.h"
#include "dde/utils.h"
#include "logger/logger.h"

namespace app::compute {

    SurfaceProfile::SurfaceProfile(
        DDEConnectionManager* connectionManager, Logger& logger)
        : m_connectionManager(connectionManager)
        , m_logger(logger)
    {
    }

    ZemaxDDE::ZemaxDDEClient* SurfaceProfile::getClient() const {
        if (m_boundClient) return m_boundClient;
        return m_connectionManager ? m_connectionManager->getActiveClient() : nullptr;
    }

    bool SurfaceProfile::isCancelled() const {
        return m_cancelRequested
            || (m_uiOpMonitor && m_taskId > 0 && m_uiOpMonitor->isCancelled(m_taskId));
    }

    void SurfaceProfile::startCalculation(
        int surface, int sampling, double angle, app::models::TaskSource source, const std::string& label)
    {
        // Bump the generation so any still-in-flight callbacks from a previous run
        // will be ignored when they arrive.
        uint64_t generation = ++m_generation;

        // Resolve the client fresh each run so a stale pointer from a previously
        // disconnected slot can never be re-used (use-after-free guard).
        m_boundClient = nullptr;

        auto* client = getClient();
        if (!client) {
            m_state = State::Failed;
            m_error = "No active DDE connection";
            if (onFailed) onFailed();
            return;
        }

        m_boundClient = client;
        m_source = source;
        if (m_createTask && m_uiOpMonitor) {
            std::string taskLabel = label.empty()
                ? (source == app::models::TaskSource::NominalSurfaceProfile ? "Nominal Profile" : "Toleranced Profile")
                : label;
            int clientSlot = m_connectionManager ? m_connectionManager->getActiveIndex() : -1;
            m_taskId = m_uiOpMonitor->startTask(source, taskLabel, sampling + 2, clientSlot);
        }

        m_state = State::FetchingSurfaceData;
        m_error.clear();
        m_targetSurface = surface;
        m_targetSampling = sampling;
        m_targetAngle = angle;
        m_sagPointIndex = 0;
        m_skippedPoints = 0;
        m_totalDdeRequests = 0;
        m_cancelRequested = false;
        m_calcStartTime = std::chrono::steady_clock::now();
        m_result = {};
        m_result.id = surface;
        m_result.sagDataPoints.clear();
        m_surfaceRequestsRemaining = 2;

        int estimatedRequests = sampling + 2;
        m_logger.addLog(std::format("[SurfaceProfileService] Starting: surface {} ({} pts, {}°) — estimated {} DDE requests",
            surface, sampling, angle, estimatedRequests));

        DWORD surfaceDataTimeout = m_surfaceDataTimeoutMsOverride > 0
            ? m_surfaceDataTimeoutMsOverride : client->getDefaultTimeoutMs();

        m_totalDdeRequests += 2;

        client->submitRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::TYPE_NAME),
            [this, generation](const std::string& result) {
                onSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::TYPE_NAME, result, generation);
            },
            [this, generation](const std::string& error) {
                onError(std::format("GetSurfaceData(TYPE_NAME): {}", error), generation);
            },
            surfaceDataTimeout, 1, "SurfaceProfileService");

        client->submitRequest(
            std::format("GetSurfaceData,{},{}", surface, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER),
            [this, generation](const std::string& result) {
                onSurfaceDataReceived(ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER, result, generation);
            },
            [this, generation](const std::string& error) {
                onError(std::format("GetSurfaceData(SEMI_DIAMETER): {}", error), generation);
            },
            surfaceDataTimeout, 1, "SurfaceProfileService");
    }

    void SurfaceProfile::cancel() {
        m_cancelRequested = true;
        m_boundClient = nullptr;
        if (m_uiOpMonitor && m_taskId > 0) {
            m_uiOpMonitor->requestCancel(m_taskId);
        }
    }

    void SurfaceProfile::onSurfaceDataReceived(
        int code, const std::string& value, uint64_t generation)
    {
        if (generation != m_generation || m_state == State::Failed || m_state == State::Completed) return;
        auto tokens = ZemaxDDE::tokenize(value);
        if (tokens.empty()) {
            onError(std::format("GetSurfaceData({}): empty response", code), generation);
            return;
        }

        if (code == ZemaxDDE::SurfaceDataCode::TYPE_NAME) {
            m_result.type = tokens[0];
        } else if (code == ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER) {
            try {
                m_result.semiDiameter = std::stod(tokens[0]);
                if (!std::isfinite(m_result.semiDiameter)) {
                    onError("GetSurfaceData(SEMI_DIAMETER): non-finite value", generation);
                    return;
                }
            } catch (...) {
                onError("GetSurfaceData(SEMI_DIAMETER): invalid number", generation);
                return;
            }
        }

        if (--m_surfaceRequestsRemaining > 0) return;

        if (onProgress) {
            onProgress(2, m_targetSampling + 2, "Surface data loaded");
        }

        m_state = State::FetchingSagPoints;
        sendNextSagRequest();
    }

    void SurfaceProfile::sendNextSagRequest() {
        if (m_sagPointIndex >= m_targetSampling) {
            m_result.sampling = m_targetSampling;
            m_result.angle = m_targetAngle;
            m_state = State::Completed;
            // Drop the reference to the client at completion so a later disconnect
            // cannot leave 'this' holding a dangling client pointer.
            m_boundClient = nullptr;

            if (m_uiOpMonitor) {
                m_uiOpMonitor->completeTask(m_taskId);
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_calcStartTime);
            if (m_skippedPoints > 0) {
                m_logger.addLog(std::format("[SurfaceProfileService] Completed: {}/{} points ({} skipped) in {} — {} DDE requests",
                    m_result.sagDataPoints.size(), m_targetSampling, m_skippedPoints,
                    ZemaxDDE::formatDuration(elapsed), m_totalDdeRequests));
            } else {
                m_logger.addLog(std::format("[SurfaceProfileService] Completed: {}/{} points in {} — {} DDE requests",
                    m_result.sagDataPoints.size(), m_targetSampling,
                    ZemaxDDE::formatDuration(elapsed), m_totalDdeRequests));
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
            m_logger.addLog("[SurfaceProfileService] Cancelled by user");
            if (onFailed) onFailed();
            return;
        }

        if (m_uiOpMonitor) {
            m_uiOpMonitor->reportProgress(m_taskId, 2 + m_sagPointIndex,
                std::format("Point {}/{}", m_sagPointIndex, m_targetSampling));
        }

        if (onProgress) {
            onProgress(2 + m_sagPointIndex, m_targetSampling + 2,
                std::format("Point {}/{}", m_sagPointIndex, m_targetSampling));
        }

        if (m_targetSampling <= 1) {
            m_result.sagDataPoints.clear();
            m_state = State::Completed;
            m_boundClient = nullptr;
            if (onComplete) onComplete();
            return;
        }

        const double rad = m_targetAngle * ZemaxDDE::DEG_TO_RAD;
        double semiDiameter = m_result.semiDiameter;
        double step = 2.0 * semiDiameter / (m_targetSampling - 1);
        double r = -semiDiameter + m_sagPointIndex * step;
        double x = r * std::cos(rad);
        double y = r * std::sin(rad);

        auto* client = getClient();
        if (!client) {
            onError("Connection lost during calculation", m_generation);
            return;
        }

        DWORD sagTimeout = m_sagTimeoutMsOverride > 0
            ? m_sagTimeoutMsOverride : client->getDefaultTimeoutMs();

        m_totalDdeRequests++;

        uint64_t generation = m_generation;
        client->submitRequest(
            std::format("GetSag,{},{},{}", m_targetSurface, x, y),
            [this, generation](const std::string& result) {
                onSagDataReceived(result, generation);
            },
            [this, generation](const std::string& error) {
                if (error == "Timeout") {
                    onSagTimeout(generation);
                } else {
                    onError(std::format("GetSag failed: {}", error), generation);
                }
            },
            sagTimeout, 1, "SurfaceProfileService");
    }

    void SurfaceProfile::onSagDataReceived(const std::string& buffer, uint64_t generation) {
        if (generation != m_generation || m_state == State::Failed || m_state == State::Completed) return;
        auto tokens = ZemaxDDE::tokenize(buffer);
        if (tokens.size() < 2) {
            onError("GetSag: invalid response format", generation);
            return;
        }

        const double rad = m_targetAngle * ZemaxDDE::DEG_TO_RAD;
        double semiDiameter = m_result.semiDiameter;
        double step = 2.0 * semiDiameter / (m_targetSampling - 1);
        double r = -semiDiameter + m_sagPointIndex * step;

        try {
            app::models::SagData point;
            point.x = r * std::cos(rad);
            point.y = r * std::sin(rad);
            point.sag = std::stod(tokens[0]);
            point.alternateSag = std::stod(tokens[1]);

            // Filter NaN/Inf so they never enter the result set (they would skew
            // P-V and deviation plots downstream).
            if (!std::isfinite(point.sag) || !std::isfinite(point.alternateSag) ||
                !std::isfinite(point.x) || !std::isfinite(point.y)) {
                m_logger.addLog(std::format("[SurfaceProfileService] Point {} has non-finite sag, skipping",
                    m_sagPointIndex));
                m_skippedPoints++;
                m_sagPointIndex++;
                sendNextSagRequest();
                return;
            }

            m_result.sagDataPoints.push_back(point);
        } catch (...) {
            onError("GetSag: failed to parse sag values", generation);
            return;
        }

        m_sagPointIndex++;
        sendNextSagRequest();
    }

    void SurfaceProfile::onSagTimeout(uint64_t generation) {
        if (generation != m_generation || m_state == State::Failed || m_state == State::Completed) return;
        m_logger.addLog(std::format("[SurfaceProfileService] Point {} timed out, skipping", m_sagPointIndex));
        m_skippedPoints++;
        m_sagPointIndex++;
        sendNextSagRequest();
    }

    void SurfaceProfile::onError(const std::string& error, uint64_t generation) {
        if (generation != m_generation || m_state == State::Failed || m_state == State::Completed) return;
        m_state = State::Failed;
        m_error = error;
        m_boundClient = nullptr;

        if (m_uiOpMonitor) {
            m_uiOpMonitor->failTask(m_taskId, error);
        }

        m_logger.addLog(std::format("[SurfaceProfileService] {}", error));
        if (onFailed) onFailed();
    }

}
