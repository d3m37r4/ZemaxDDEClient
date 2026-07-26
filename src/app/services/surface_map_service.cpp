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
        , m_nominalCalculator(connectionManager, logger)
        , m_mapCalculator(connectionManager, logger)
    {
    }

    void SurfaceMapService::setUiOperationMonitor(app::services::OperationMonitorService* monitor) {
        m_uiOpMonitor = monitor;
        m_nominalCalculator.setMonitor(monitor);
        m_mapCalculator.setMonitor(monitor);
    }

    void SurfaceMapService::startCalculation(int surface, int sampling, double angle, app::models::TaskSource source) {
        m_nominalCalcSlot = m_connectionManager ? m_connectionManager->getActiveIndex() : -1;
        if (m_connectionManager) {
            m_nominalCalculator.setSurfaceDataTimeoutMs(m_connectionManager->getGetSurfaceDataMapTimeoutMs());
            m_nominalCalculator.setSagTimeoutMs(m_connectionManager->getGetSagMapTimeoutMs());
        }

        m_nominalCalculator.onComplete = [this]() {
            m_nominalSurfaceData = m_nominalCalculator.getResult();
            m_nominalCalcSlot = -1;
            if (m_nominalSurfaceData.isValid()) {
                m_logger.addLog("[IrregularityMapService] Nominal surface profile calculated and stored as reference");
            }
            if (onNominalCalculationComplete) onNominalCalculationComplete();
        };
        m_nominalCalculator.onFailed = [this]() {
            m_nominalCalcSlot = -1;
            if (onNominalCalculationComplete) onNominalCalculationComplete();
        };

        m_nominalCalculator.startCalculation(surface, sampling, angle, source);
    }

    void SurfaceMapService::cancelCalculation() {
        m_nominalCalculator.cancel();
        m_nominalCalcSlot = -1;
    }

    void SurfaceMapService::startMapCalculation(int surface, int sampling, double angleStepDeg) {
        auto* client = m_connectionManager ? m_connectionManager->getActiveClient() : nullptr;
        if (!client) {
            m_logger.addLog("[IrregularityMapService] No active DDE connection");
            return;
        }

        m_mapCalcSlot = m_connectionManager->getActiveIndex();

        if (m_connectionManager) {
            m_mapCalculator.setSurfaceDataTimeoutMs(m_connectionManager->getGetSurfaceDataMapTimeoutMs());
            m_mapCalculator.setSagTimeoutMs(m_connectionManager->getGetSagMapTimeoutMs());
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
        m_totalDdeRequests = 0;
        m_mapTaskId = 0;
        m_calcStartTime = std::chrono::steady_clock::now();

        int estimatedRequests = m_totalAngles * (sampling + 2);
        m_logger.addLog(std::format("[IrregularityMapService] Starting surface map: surface {}, {} sections ({}° step, {} pts each) — estimated {} DDE requests",
            surface, m_totalAngles, angleStepDeg, sampling, estimatedRequests));

        if (m_uiOpMonitor) {
            int clientSlot = m_connectionManager ? m_connectionManager->getActiveIndex() : -1;
            m_mapTaskId = m_uiOpMonitor->startTask(
                app::models::TaskSource::SurfaceIrregularityMap, "Surface Irregularity Map",
                m_totalAngles * (m_targetSampling + 2), clientSlot);
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

            m_mapCalcSlot = -1;
            m_windowState.tolerancedSurfaceIndex = m_targetSurface;
            m_windowState.tolerancedSampling = m_targetSampling;
            m_windowState.tolerancedAngleStep = m_angleStepDeg;

            m_logger.addLog(std::format("[IrregularityMapService] Surface map completed: {} sections in {} — {} DDE requests",
                m_profiles.size(), ZemaxDDE::formatDuration(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - m_calcStartTime)), m_totalDdeRequests));

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

        m_mapCalculator.m_createTask = false;

        m_mapCalculator.onProgress = [this](int cur, int /*total*/, const std::string& /*msg*/) {
            if (m_uiOpMonitor && m_mapTaskId > 0) {
                std::string sectionMsg = std::format("Section {}/{}",
                    m_currentAngleIndex + 1, m_totalAngles);
                m_uiOpMonitor->reportProgress(m_mapTaskId, m_totalDdeRequests + cur, sectionMsg);
            }
        };

        m_mapCalculator.onComplete = [this, angle]() {
            auto& profile = m_mapCalculator.getResult();

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
            m_totalDdeRequests += m_mapCalculator.getTotalDdeRequests();
            m_currentAngleIndex++;

            startNextProfile();
        };

        m_mapCalculator.onFailed = [this]() {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_calcStartTime);
            if (m_mapCalculator.isCancelled()) {
                m_logger.addLog(std::format("[IrregularityMapService] Surface map cancelled in {}",
                    ZemaxDDE::formatDuration(elapsed)));
            } else {
                m_logger.addLog(std::format("[IrregularityMapService] Profile calculation failed after {}: {}",
                    ZemaxDDE::formatDuration(elapsed), m_mapCalculator.getError()));
            }
            if (m_uiOpMonitor && m_mapTaskId > 0) {
                m_uiOpMonitor->failTask(m_mapTaskId, m_mapCalculator.getError().empty() ? "Cancelled" : m_mapCalculator.getError());
                m_mapTaskId = 0;
            }
            m_profiles.clear();
            m_currentAngleIndex = 0;
            m_mapCalcSlot = -1;
        };

        m_mapCalculator.startCalculation(m_targetSurface, m_targetSampling, angle,
            app::models::TaskSource::SurfaceIrregularityMap,
            std::format("Section {}/{} at {:.1f}°", m_currentAngleIndex + 1, m_totalAngles, angle));
    }

    void SurfaceMapService::cancelMapCalculation() {
        if (m_mapCalculator.isCalculating()) {
            m_mapCalculator.cancel();
        } else if (m_mapTaskId > 0) {
            if (m_uiOpMonitor) m_uiOpMonitor->failTask(m_mapTaskId, "Cancelled");
            m_mapTaskId = 0;
            m_profiles.clear();
            m_currentAngleIndex = 0;
        }
        m_mapCalcSlot = -1;
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
