#include <algorithm>
#include <cmath>
#include <format>

#include "dde/constants.h"
#include "dde/operation_monitor.h"
#include "dde/utils.h"

#include "app/services/surface_map_service.h"
#include "logger/logger.h"

namespace app::services {
    SurfaceMapService::SurfaceMapService(DDEConnectionManager* connectionManager, Logger& logger)
        : m_connectionManager(connectionManager)
        , m_logger(logger)
        , m_calculator(connectionManager, logger)
    {
    }

    void SurfaceMapService::setUiOperationMonitor(app::services::OperationMonitorService* monitor) {
        m_uiOpMonitor = monitor;
        m_calculator.setMonitor(monitor);
    }

    void SurfaceMapService::startCalculation(int surface, int sampling, double angle, app::models::TaskSource source) {
        if (m_connectionManager) {
            m_calculator.setSurfaceDataTimeoutMs(m_connectionManager->getGetSurfaceDataMapTimeoutMs());
            m_calculator.setSagTimeoutMs(m_connectionManager->getGetSagMapTimeoutMs());
        }

        m_calculator.onComplete = [this]() {
            m_nominalSurfaceData = m_calculator.getResult();
            if (m_nominalSurfaceData.isValid()) {
                m_logger.addLog("[IrregularityMapService] Nominal surface profile calculated and stored as reference");
            }
            if (onCalculationComplete) onCalculationComplete();
        };
        m_calculator.onFailed = [this]() {
            if (onCalculationComplete) onCalculationComplete();
        };

        m_calculator.startCalculation(surface, sampling, angle, source);
    }

    void SurfaceMapService::cancelCalculation() {
        m_calculator.cancel();
    }

    void SurfaceMapService::startMapCalculation(int surface, int sampling, double angleStepDeg) {
        auto* client = m_connectionManager ? m_connectionManager->getActiveClient() : nullptr;
        if (!client) {
            m_logger.addLog("[IrregularityMapService] No active DDE connection");
            return;
        }

        if (m_connectionManager) {
            m_calculator.setSurfaceDataTimeoutMs(m_connectionManager->getGetSurfaceDataMapTimeoutMs());
            m_calculator.setSagTimeoutMs(m_connectionManager->getGetSagMapTimeoutMs());
        }

        if (angleStepDeg <= 0.0 || angleStepDeg >= 180.0) {
            m_logger.addLog("[IrregularityMapService] Invalid angle step, must be in range (0, 180)");
            return;
        }

        if (sampling < 3) {
            m_logger.addLog("[IrregularityMapService] Invalid sampling, must be >= 3");
            return;
        }

        m_profiles.clear();
        m_maxPVResult.reset();
        m_targetSurface = surface;
        m_targetSampling = sampling;
        m_angleStepDeg = angleStepDeg;
        m_totalAngles = static_cast<int>(180.0 / angleStepDeg);
        m_currentAngleIndex = 0;
        m_centerSagRef = 0.0;
        m_mapTaskId = 0;
        m_calcStartTime = std::chrono::steady_clock::now();

        m_logger.addLog(std::format("[IrregularityMapService] Starting surface map: surface {}, {} sections ({}° step, {} pts each)",
            surface, m_totalAngles, angleStepDeg, sampling));

        if (m_uiOpMonitor) {
            m_mapTaskId = m_uiOpMonitor->startTask(
                app::models::TaskSource::SurfaceIrregularityMap, "Surface Irregularity Map", m_totalAngles);
        }

        startNextProfile();
    }

    void SurfaceMapService::startNextProfile() {
        if (m_uiOpMonitor && m_mapTaskId > 0 && m_uiOpMonitor->isCancelled(m_mapTaskId)) {
            if (m_uiOpMonitor) m_uiOpMonitor->failTask(m_mapTaskId, "Cancelled");
            m_mapTaskId = 0;
            m_profiles.clear();
            m_logger.addLog("[IrregularityMapService] Surface map cancelled");
            return;
        }

        if (m_currentAngleIndex >= m_totalAngles) {
            if (m_uiOpMonitor && m_mapTaskId > 0) {
                m_uiOpMonitor->completeTask(m_mapTaskId);
                m_mapTaskId = 0;
            }

            m_windowState.tolerancedSurfaceIndex = m_targetSurface;
            m_windowState.tolerancedSampling = m_targetSampling;
            m_windowState.tolerancedAngleStep = m_angleStepDeg;

            m_logger.addLog(std::format("[IrregularityMapService] Surface map completed: {} sections in {}",
                m_profiles.size(), ZemaxDDE::formatDuration(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - m_calcStartTime))));

            if (m_nominalSurfaceData.isValid()) {
                m_maxPVResult = findMaxPVSection();
                if (m_maxPVResult) {
                    m_logger.addLog(std::format("[IrregularityMapService] Max P-V section: {:.2f}°, P={:.6f}, V={:.6f}, PV={:.6f}",
                        m_maxPVResult->angle, m_maxPVResult->peak, m_maxPVResult->valley, m_maxPVResult->pv));
                }
            }
            return;
        }

        double angle = m_currentAngleIndex * m_angleStepDeg;

        m_calculator.onComplete = [this, angle]() {
            auto& profile = m_calculator.getResult();

            if (m_currentAngleIndex == 0) {
                m_centerSagRef = profile.sagDataPoints.empty() ? 0.0
                    : profile.sagDataPoints[(profile.sagDataPoints.size() - 1) / 2].sag;
            } else if (!profile.sagDataPoints.empty()) {
                auto centerIdx = (profile.sagDataPoints.size() - 1) / 2;
                auto centerSag = profile.sagDataPoints[centerIdx].sag;
                if (std::abs(centerSag - m_centerSagRef) > 1e-12) {
                    m_logger.addLog(std::format("[IrregularityMapService] WARNING: centre sag mismatch at {:.2f}°", angle));
                }
            }

            m_profiles.push_back(profile);
            m_currentAngleIndex++;

            if (m_uiOpMonitor && m_mapTaskId > 0) {
                m_uiOpMonitor->reportProgress(m_mapTaskId, m_currentAngleIndex,
                    std::format("Section {}/{} at {:.1f}°", m_currentAngleIndex, m_totalAngles, angle));
            }

            startNextProfile();
        };

        m_calculator.onFailed = [this]() {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_calcStartTime);
            if (m_calculator.isCancelled()) {
                m_logger.addLog(std::format("[IrregularityMapService] Surface map cancelled in {}",
                    ZemaxDDE::formatDuration(elapsed)));
            } else {
                m_logger.addLog(std::format("[IrregularityMapService] Profile calculation failed after {}: {}",
                    ZemaxDDE::formatDuration(elapsed), m_calculator.getError()));
            }
            if (m_uiOpMonitor && m_mapTaskId > 0) {
                m_uiOpMonitor->failTask(m_mapTaskId, m_calculator.getError().empty() ? "Cancelled" : m_calculator.getError());
                m_mapTaskId = 0;
            }
            m_profiles.clear();
            m_currentAngleIndex = 0;
        };

        m_calculator.startCalculation(m_targetSurface, m_targetSampling, angle,
            app::models::TaskSource::SurfaceIrregularityMap,
            std::format("Section {}/{} at {:.1f}°", m_currentAngleIndex + 1, m_totalAngles, angle));
    }

    void SurfaceMapService::cancelMapCalculation() {
        if (m_calculator.isCalculating()) {
            m_calculator.cancel();
        } else if (m_mapTaskId > 0) {
            if (m_uiOpMonitor) m_uiOpMonitor->failTask(m_mapTaskId, "Cancelled");
            m_mapTaskId = 0;
            m_profiles.clear();
            m_currentAngleIndex = 0;
        }
    }

    void SurfaceMapService::clearData() {
        m_profiles.clear();
        m_maxPVResult.reset();
        m_currentAngleIndex = 0;
        m_mapTaskId = 0;
    }

    std::optional<MaxPVResult> SurfaceMapService::findMaxPVSection() const {
        return m_mapEngine.findMaxPVSection(m_profiles, m_nominalSurfaceData);
    }
}
