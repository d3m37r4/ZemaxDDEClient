#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

#include "dde/client.h"
#include "dde/constants.h"
#include "dde/lens_update.h"
#include "dde/operation_monitor.h"
#include "dde/utils.h"

#include "app/services/surface_map_service.h"
#include "logger/logger.h"

namespace app::services {
    namespace {
        // Unique ids so cancelling the map never drops the nominal refresh.
        constexpr const char* kMapRefreshId = "LensRefresh#map";
        constexpr const char* kMapNominalRefreshId = "LensRefresh#mapNominal";
    }
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

    void SurfaceMapService::onClientDisconnected(int index) {
        if (m_nominalCalcSlot == index) {
            abortPendingRefresh(true);
            cancelPendingRefreshTask(true);
            m_nominalCalculator.cancel();
            m_nominalCalcSlot = -1;
            m_nominalSurfaceData = {};
        }
        if (m_mapCalcSlot == index) {
            abortPendingRefresh(false);
            cancelPendingRefreshTask(false);
            m_mapCalculator.cancel();
            m_mapTaskId = 0;
            m_currentAngleIndex = 0;
            m_profiles.clear();
            m_maxPVResult.reset();
            m_mapCalcSlot = -1;
        }
        recomputeRanges();
    }

    ZemaxDDE::ZemaxDDEClient* SurfaceMapService::resolveClient(int slot) const {
        if (!m_connectionManager || slot < 0) return nullptr;
        DDEConnection* conn = m_connectionManager->getConnection(slot);
        return (conn && conn->isConnected()) ? conn->client.get() : nullptr;
    }

    void SurfaceMapService::abortPendingRefresh(bool nominal) {
        if (nominal) {
            ++m_nominalRefreshToken;
        } else {
            ++m_mapRefreshToken;
        }
        const int slot = nominal ? m_nominalCalcSlot : m_mapCalcSlot;
        if (!m_connectionManager || slot < 0) return;
        DDEConnection* conn = m_connectionManager->getConnection(slot);
        if (!conn || !conn->client) return;
        conn->client->cancelPendingRequests(nominal ? kMapNominalRefreshId : kMapRefreshId);
    }

    void SurfaceMapService::startCalculation(int surface, int sampling, double angle, app::models::TaskSource source) {
        m_nominalCalcSlot = m_connectionManager ? m_connectionManager->getActiveIndex() : -1;
        if (m_connectionManager) {
            m_nominalCalculator.setSurfaceDataTimeoutMs(m_connectionManager->getGetSurfaceDataMapTimeoutMs());
            m_nominalCalculator.setSagTimeoutMs(m_connectionManager->getGetSagMapTimeoutMs());
        }

        m_nominalCalculator.onComplete = [this]() {
            m_nominalSurfaceData = m_nominalCalculator.getResult();
            // Stick the slot to the data before it is cleared: detached plot
            // titles render "[Slot N]" from it.
            m_nominalSurfaceData.sourceSlot = m_nominalCalcSlot;
            m_nominalCalcSlot = -1;
            recomputeRanges();
            if (m_nominalSurfaceData.isValid()) {
                m_logger.addLog("[IrregularityMapService] Nominal surface profile calculated and stored as reference");
            }
            if (onNominalCalculationComplete) onNominalCalculationComplete();
        };
        m_nominalCalculator.onFailed = [this]() {
            m_nominalCalcSlot = -1;
            // Do not publish the aborted run's partial result on cancel; see
            // SurfaceProfileService::onNominalFailed for the same contract.
            if (m_nominalCalculator.isCancelled()) return;
            if (onNominalCalculationComplete) onNominalCalculationComplete();
        };

        // Refresh the server lens copy first; the profile starts in the callback.
        const uint64_t token = ++m_nominalRefreshToken;
        ZemaxDDE::ZemaxDDEClient* client = resolveClient(m_nominalCalcSlot);
        if (!client) {
            // Let the calculator report the failure through onFailed as before.
            m_nominalCalculator.startCalculation(surface, sampling, angle, source, "", m_nominalCalcSlot);
            return;
        }

        const int targetSlot = m_nominalCalcSlot;
        // Register the task before the refresh so the phase is visible and
        // cancellable; the calculator adopts the same id on success.
        if (m_uiOpMonitor) {
            m_nominalRefreshTaskId = m_uiOpMonitor->startTask(
                source, "Nominal Profile", sampling + 2, targetSlot,
                client->getOperationMonitor());
        }

        requestLensRefresh(*client, m_logger,
            [this, token, targetSlot, surface, sampling, angle, source]
            (ZemaxDDE::UpdateStatus status) {
                if (m_nominalRefreshToken != token) return;
                if (status == ZemaxDDE::UpdateStatus::NotUpdated) {
                    m_logger.addLog("[IrregularityMapService] GetRefresh returned \"-1\": nominal profile aborted");
                    if (m_uiOpMonitor && m_nominalRefreshTaskId > 0) {
                        m_uiOpMonitor->failTask(m_nominalRefreshTaskId,
                            "GetRefresh returned \"-1\": the lens cannot be updated");
                        m_nominalRefreshTaskId = 0;
                    }
                    m_nominalCalcSlot = -1;
                    if (onUpdateFailed) {
                        onUpdateFailed("Zemax returned \"-1\" for GetRefresh — the lens cannot be "
                                       "updated, so the calculation was stopped.");
                    }
                    return;
                }
                if (status == ZemaxDDE::UpdateStatus::Unknown) {
                    m_logger.addLog("[IrregularityMapService] WARNING: GetRefresh gave no usable "
                                    "answer; continuing with the current server lens");
                }
                const uint64_t taskId = m_nominalRefreshTaskId;
                m_nominalRefreshTaskId = 0;
                if (taskId > 0) m_nominalCalculator.adoptTask(taskId);
                m_nominalCalculator.startCalculation(surface, sampling, angle, source, "", targetSlot);
            },
            client->getGetRefreshTimeoutMs(), 1, kMapNominalRefreshId);
    }

    void SurfaceMapService::cancelPendingRefreshTask(bool nominal) {
        if (!m_uiOpMonitor) return;
        uint64_t& taskId = nominal ? m_nominalRefreshTaskId : m_mapRefreshTaskId;
        if (taskId > 0) {
            m_uiOpMonitor->failTask(taskId, "Cancelled");
            taskId = 0;
        }
    }

    void SurfaceMapService::cancelCalculation() {
        abortPendingRefresh(true);
        cancelPendingRefreshTask(true);
        m_nominalCalculator.cancel();
        m_nominalCalcSlot = -1;
    }

    void SurfaceMapService::startMapCalculation(int surface, int sampling, double angleStepDeg) {
        auto* client = m_connectionManager ? m_connectionManager->getActiveClient() : nullptr;
        if (!client) {
            m_logger.addLog("[IrregularityMapService] No active DDE connection");
            return;
        }

        // The plan clamps the step and the sampling so that the DDE load stays
        // inside the request budget; the effective values are what the run uses.
        const app::compute::MapPlan plan = app::compute::planSurfaceMap(angleStepDeg, sampling);
        if (plan.adjusted) {
            m_logger.addLog(std::format(
                "[IrregularityMapService] Adjusted for the request budget: step {:.3f}° -> {:.3f}°, sampling {} -> {}",
                angleStepDeg, plan.stepDeg, sampling, plan.sampling));
        }

        // Guard: if a map is already in flight, cancel it before starting a new one
        // so old async callbacks cannot contaminate the fresh run.
        if (m_mapCalculator.isCalculating()) {
            m_mapCalculator.cancel();
        }
        if (m_uiOpMonitor && m_mapTaskId > 0) {
            m_uiOpMonitor->failTask(m_mapTaskId, "Restarted");
            m_mapTaskId = 0;
        }

        m_mapCalcSlot = m_connectionManager->getActiveIndex();

        // Register the task before the refresh so the phase is visible and
        // cancellable; beginMapRun() adopts the same id on success.
        if (m_uiOpMonitor) {
            m_mapRefreshTaskId = m_uiOpMonitor->startTask(
                app::models::TaskSource::SurfaceIrregularityMap, "Surface Irregularity Map",
                plan.sections * (plan.sampling + 2), m_mapCalcSlot,
                client->getOperationMonitor());
        }

        // Refresh the server lens copy first; the section loop starts only when
        // the refresh did not report a model Zemax cannot update.
        const uint64_t token = ++m_mapRefreshToken;
        requestLensRefresh(*client, m_logger,
            [this, token, plan, surface](ZemaxDDE::UpdateStatus status) {
                if (m_mapRefreshToken != token) return;   // cancelled or restarted
                if (status == ZemaxDDE::UpdateStatus::NotUpdated) {
                    m_logger.addLog("[IrregularityMapService] GetRefresh returned \"-1\": map aborted");
                    if (m_uiOpMonitor && m_mapRefreshTaskId > 0) {
                        m_uiOpMonitor->failTask(m_mapRefreshTaskId,
                            "GetRefresh returned \"-1\": the lens cannot be updated");
                        m_mapRefreshTaskId = 0;
                    }
                    m_mapCalcSlot = -1;
                    if (onUpdateFailed) {
                        onUpdateFailed("Zemax returned \"-1\" for GetRefresh — the lens cannot be "
                                       "updated, so the calculation was stopped.");
                    }
                    return;
                }
                if (status == ZemaxDDE::UpdateStatus::Unknown) {
                    m_logger.addLog("[IrregularityMapService] WARNING: GetRefresh gave no usable "
                                    "answer; continuing with the current server lens");
                }
                const uint64_t taskId = m_mapRefreshTaskId;
                m_mapRefreshTaskId = 0;
                beginMapRun(plan, surface, taskId);
            },
            client->getGetRefreshTimeoutMs(), 1, kMapRefreshId);
    }

    void SurfaceMapService::beginMapRun(const app::compute::MapPlan& plan, int surface, uint64_t taskId) {
        if (m_connectionManager) {
            m_mapCalculator.setSurfaceDataTimeoutMs(m_connectionManager->getGetSurfaceDataMapTimeoutMs());
            m_mapCalculator.setSagTimeoutMs(m_connectionManager->getGetSagMapTimeoutMs());
        }

        m_profiles.clear();
        m_maxPVResult.reset();
        m_targetSurface = surface;
        m_targetSampling = plan.sampling;
        m_angleStepDeg = plan.stepDeg;
        m_totalAngles = plan.sections;
        m_currentAngleIndex = 0;
        m_centerSagRef = 0.0;
        m_totalDdeRequests = 0;
        m_mapTaskId = taskId;
        m_calcStartTime = std::chrono::steady_clock::now();

        m_logger.addLog(std::format("[IrregularityMapService] Starting surface map: surface {}, {} sections ({:.3f}° step, {} pts each) — estimated {} DDE requests",
            surface, m_totalAngles, plan.stepDeg, plan.sampling, plan.estimatedRequests));

        if (m_uiOpMonitor && m_mapTaskId == 0) {
            // Fallback for callers that did not pre-register a task.
            m_mapTaskId = m_uiOpMonitor->startTask(
                app::models::TaskSource::SurfaceIrregularityMap, "Surface Irregularity Map",
                m_totalAngles * (m_targetSampling + 2), m_mapCalcSlot);
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
            recomputeRanges();

            m_logger.addLog(std::format("[IrregularityMapService] Surface map completed: {} sections in {} — {} DDE requests",
                m_profiles.size(), ZemaxDDE::formatDuration(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - m_calcStartTime)), m_totalDdeRequests));
            if (onMapCalculationComplete) {
                onMapCalculationComplete();
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
            const auto& profile = m_mapCalculator.getResult();
            if (m_currentAngleIndex == 0) {
                m_centerSagRef = profile.sagDataPoints.empty() ? 0.0
                    : profile.sagDataPoints[(profile.sagDataPoints.size() - 1) / 2].sag;
            } else if (!profile.sagDataPoints.empty()) {
                auto centerIdx = (profile.sagDataPoints.size() - 1) / 2;
                auto centerSag = profile.sagDataPoints[centerIdx].sag;
                if (std::abs(centerSag - m_centerSagRef) > 1e-12) {
                    m_logger.addLog(std::format("[IrregularityMapService] WARNING: centre sag mismatch at {:.3f}°", angle));
                }
            }

            // Every section of a run comes from the same slot; stamp each
            // profile so titles resolve after the run ends (m_mapCalcSlot
            // resets to -1 once the map completes).
            app::models::SurfaceData completed = profile;
            completed.sourceSlot = m_mapCalcSlot;
            m_profiles.push_back(std::move(completed));
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
            std::format("Section {}/{} at {:.3f}°", m_currentAngleIndex + 1, m_totalAngles, angle),
            m_mapCalcSlot);
    }

    void SurfaceMapService::cancelMapCalculation() {
        abortPendingRefresh(false);
        cancelPendingRefreshTask(false);
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

    bool SurfaceMapService::cancelTask(uint64_t taskId) {
        if (taskId == 0) return false;
        // Tasks created for the refresh phase have not reached the calculator.
        if (m_mapRefreshTaskId == taskId) {
            cancelMapCalculation();
            return true;
        }
        if (m_nominalRefreshTaskId == taskId) {
            cancelCalculation();
            return true;
        }
        if (m_mapTaskId > 0 && m_mapTaskId == taskId) {
            cancelMapCalculation();
            return true;
        }
        if (m_nominalCalculator.taskId() == taskId && m_nominalCalculator.isCalculating()) {
            cancelCalculation();
            return true;
        }
        return false;
    }

    void SurfaceMapService::clearData() {
        m_profiles.clear();
        m_maxPVResult.reset();
        m_currentAngleIndex = 0;
        m_mapTaskId = 0;
        recomputeRanges();
    }

    std::optional<MaxPVResult> SurfaceMapService::findMaxPVSection() const {
        return m_mapEngine.findMaxPVSection(m_profiles, m_nominalSurfaceData);
    }

    bool SurfaceMapService::calculateDeviations() {
        if (!hasData() || !m_nominalSurfaceData.isValid()) return false;
        m_maxPVResult = findMaxPVSection();
        if (m_maxPVResult) {
            m_logger.addLog(std::format("[IrregularityMapService] Max P-V section: {:.2f}°, P={:.6f}, V={:.6f}, PV={:.6f}",
                m_maxPVResult->angle, m_maxPVResult->peak, m_maxPVResult->valley, m_maxPVResult->pv));
        }
        return m_maxPVResult.has_value();
    }

    void SurfaceMapService::recomputeRanges() {
        m_ranges = {};

        if (!m_profiles.empty()) {
            float lo = std::numeric_limits<float>::max();
            float hi = std::numeric_limits<float>::lowest();
            bool any = false;
            for (const auto& profile : m_profiles) {
                for (const auto& pt : profile.sagDataPoints) {
                    const float sag = static_cast<float>(pt.sag);
                    lo = std::min(lo, sag);
                    hi = std::max(hi, sag);
                    any = true;
                }
            }
            if (any) {
                m_ranges.sagValid = true;
                m_ranges.sagMin = lo;
                m_ranges.sagMax = hi;
            }
        }

        if (!m_profiles.empty() && m_nominalSurfaceData.isValid()) {
            float lo = std::numeric_limits<float>::max();
            float hi = std::numeric_limits<float>::lowest();
            bool any = false;
            for (const auto& profile : m_profiles) {
                const size_t n = std::min(profile.sagDataPoints.size(),
                                          m_nominalSurfaceData.sagDataPoints.size());
                for (size_t i = 0; i < n; ++i) {
                    const float dev = static_cast<float>(
                        profile.sagDataPoints[i].sag - m_nominalSurfaceData.sagDataPoints[i].sag);
                    lo = std::min(lo, dev);
                    hi = std::max(hi, dev);
                    any = true;
                }
            }
            if (any) {
                m_ranges.devValid = true;
                m_ranges.devMin = lo;
                m_ranges.devMax = hi;
            }
        }
    }
}
