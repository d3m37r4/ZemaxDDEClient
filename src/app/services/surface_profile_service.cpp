#include <cmath>
#include <format>

#include "dde/client.h"
#include "dde/lens_update.h"
#include "dde/utils.h"

#include "app/services/surface_profile_service.h"
#include "logger/logger.h"

namespace app::services {
    namespace {
        // Unique service ids so cancelling one calculator never drops the other
        // one's pending refresh.
        constexpr const char* kRefreshIdNominal = "LensRefresh#profileNominal";
        constexpr const char* kRefreshIdToleranced = "LensRefresh#profileToleranced";
    }
    SurfaceProfileService::SurfaceProfileService(DDEConnectionManager* connectionManager, Logger& logger)
        : m_connectionManager(connectionManager)
        , m_logger(logger)
        , m_nominalCalculator(connectionManager, logger)
        , m_tolerancedCalculator(connectionManager, logger)
    {
    }

    void SurfaceProfileService::setUiOperationMonitor(app::services::OperationMonitorService* monitor) {
        m_uiOpMonitor = monitor;
        m_nominalCalculator.setMonitor(monitor);
        m_tolerancedCalculator.setMonitor(monitor);
    }

    void SurfaceProfileService::onClientDisconnected(int index) {
        if (m_nominalCalcSlot == index) {
            abortPendingRefresh(true);
            cancelPendingRefreshTask(0);
            m_nominalCalculator.cancel();
            m_nominalCalcSlot = -1;
        }
        if (m_tolerancedCalcSlot == index) {
            abortPendingRefresh(false);
            cancelPendingRefreshTask(1);
            m_tolerancedCalculator.cancel();
            m_tolerancedCalcSlot = -1;
        }
    }

    std::pair<std::vector<double>, std::vector<double>> extractSagCoordinates(const app::models::SurfaceData& surface) {
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

    bool SurfaceProfileService::isCalculating(app::models::TaskSource source) const {
        if (source == app::models::TaskSource::NominalSurfaceProfile)
            return m_nominalCalculator.isCalculating();
        if (source == app::models::TaskSource::TolerancedSurfaceProfile)
            return m_tolerancedCalculator.isCalculating();
        return false;
    }

    const app::models::SurfaceData& SurfaceProfileService::getResult(app::models::TaskSource source) const {
        switch (source) {
            case app::models::TaskSource::NominalSurfaceProfile:
                return m_nominalCalculator.getResult();
            case app::models::TaskSource::TolerancedSurfaceProfile:
                return m_tolerancedCalculator.getResult();
            case app::models::TaskSource::SurfaceIrregularityMap:
            case app::models::TaskSource::None:
            default:
                // Explicit no-fall-through: unknown sources must never silently map
                // to the toleranced profile. Return the nominal result as neutral.
                return m_nominalCalculator.getResult();
        }
    }

    ZemaxDDE::ZemaxDDEClient* SurfaceProfileService::resolveClient(int slot) const {
        if (!m_connectionManager || slot < 0) return nullptr;
        DDEConnection* conn = m_connectionManager->getConnection(slot);
        return (conn && conn->isConnected()) ? conn->client.get() : nullptr;
    }

    void SurfaceProfileService::abortPendingRefresh(bool nominal) {
        ++m_refreshToken[nominal ? 0 : 1];
        const int slot = nominal ? m_nominalCalcSlot : m_tolerancedCalcSlot;
        if (!m_connectionManager || slot < 0) return;
        DDEConnection* conn = m_connectionManager->getConnection(slot);
        if (!conn || !conn->client) return;
        conn->client->cancelPendingRequests(nominal ? kRefreshIdNominal : kRefreshIdToleranced);
    }

    void SurfaceProfileService::startCalculation(int surface, int sampling, double angle, app::models::TaskSource source) {
        const bool nominal = (source == app::models::TaskSource::NominalSurfaceProfile);
        auto& calc = nominal ? m_nominalCalculator : m_tolerancedCalculator;
        int& slot = nominal ? m_nominalCalcSlot : m_tolerancedCalcSlot;

        slot = m_connectionManager ? m_connectionManager->getActiveIndex() : -1;

        if (m_connectionManager) {
            calc.setSurfaceDataTimeoutMs(m_connectionManager->getGetSurfaceDataProfileTimeoutMs());
            calc.setSagTimeoutMs(m_connectionManager->getGetSagProfileTimeoutMs());
        }

        if (nominal) {
            calc.onComplete = [this]() { onNominalComplete(); };
            calc.onFailed = [this]() { onNominalFailed(); };
        } else {
            calc.onComplete = [this]() { onTolerancedComplete(); };
            calc.onFailed = [this]() { onTolerancedFailed(); };
        }

        // Refresh the server lens copy first; the calculation starts only when
        // the refresh did not report a model Zemax cannot update.
        const int tokenIndex = nominal ? 0 : 1;
        const uint64_t token = ++m_refreshToken[tokenIndex];
        ZemaxDDE::ZemaxDDEClient* client = resolveClient(slot);
        if (!client) {
            m_logger.addLog("[SurfaceProfileService] No active DDE connection");
            if (nominal) onNominalFailed(); else onTolerancedFailed();
            return;
        }

        const int targetSlot = slot;
        const char* serviceId = nominal ? kRefreshIdNominal : kRefreshIdToleranced;

        // Register the task before the refresh: the phase must be visible in the
        // monitor and cancellable from the status bar. The calculator adopts the
        // same id on success instead of creating a second task.
        if (m_uiOpMonitor) {
            const char* taskLabel = nominal ? "Nominal Profile" : "Toleranced Profile";
            m_refreshTaskId[tokenIndex] = m_uiOpMonitor->startTask(
                source, taskLabel, sampling + 2, slot, client->getOperationMonitor());
        }

        requestLensRefresh(*client, m_logger,
            [this, nominal, token, tokenIndex, targetSlot, surface, sampling, angle, source]
            (ZemaxDDE::UpdateStatus status) {
                if (m_refreshToken[tokenIndex] != token) return;  // cancelled or restarted
                if (status == ZemaxDDE::UpdateStatus::NotUpdated) {
                    m_logger.addLog("[SurfaceProfileService] GetRefresh returned \"-1\": calculation aborted");
                    if (m_uiOpMonitor && m_refreshTaskId[tokenIndex] > 0) {
                        m_uiOpMonitor->failTask(m_refreshTaskId[tokenIndex],
                            "GetRefresh returned \"-1\": the lens cannot be updated");
                        m_refreshTaskId[tokenIndex] = 0;
                    }
                    if (nominal) m_nominalCalcSlot = -1; else m_tolerancedCalcSlot = -1;
                    if (onUpdateFailed) {
                        onUpdateFailed("Zemax returned \"-1\" for GetRefresh — the lens cannot be "
                                       "updated, so the calculation was stopped.");
                    }
                    return;
                }
                if (status == ZemaxDDE::UpdateStatus::Unknown) {
                    m_logger.addLog("[SurfaceProfileService] WARNING: GetRefresh gave no usable "
                                    "answer; continuing with the current server lens");
                }
                const uint64_t taskId = m_refreshTaskId[tokenIndex];
                m_refreshTaskId[tokenIndex] = 0;
                auto& target = nominal ? m_nominalCalculator : m_tolerancedCalculator;
                if (taskId > 0) target.adoptTask(taskId);
                target.startCalculation(surface, sampling, angle, source, "", targetSlot);
            },
            client->getGetRefreshTimeoutMs(), 1, serviceId);
    }

    void SurfaceProfileService::onNominalComplete() {
        const auto& result = m_nominalCalculator.getResult();
        m_nominalSurfaceData = result;
        // Stick the slot to the data before it is cleared: detached plot
        // titles render "[Slot N]" from it.
        m_nominalSurfaceData.sourceSlot = m_nominalCalcSlot;
        m_nominalCalcSlot = -1;
        if (onNominalCalculationComplete) onNominalCalculationComplete();
    }

    void SurfaceProfileService::onNominalFailed() {
        m_nominalCalcSlot = -1;
        // A cancellation must not publish the aborted run's partial result:
        // the complete callback copies getResult() on any failure. Genuine
        // errors keep the previous behavior (panel copy is refreshed).
        if (!m_nominalCalculator.isCancelled() && onNominalCalculationComplete) {
            onNominalCalculationComplete();
        }
    }

    void SurfaceProfileService::onTolerancedComplete() {
        const auto& result = m_tolerancedCalculator.getResult();
        m_tolerancedSurfaceData = result;
        // See onNominalComplete: the slot travels with the data.
        m_tolerancedSurfaceData.sourceSlot = m_tolerancedCalcSlot;
        m_tolerancedCalcSlot = -1;
        if (onTolerancedCalculationComplete) onTolerancedCalculationComplete();
    }

    void SurfaceProfileService::onTolerancedFailed() {
        m_tolerancedCalcSlot = -1;
        // See onNominalFailed: a cancellation must not publish partial data.
        if (!m_tolerancedCalculator.isCancelled() && onTolerancedCalculationComplete) {
            onTolerancedCalculationComplete();
        }
    }

    void SurfaceProfileService::cancelPendingRefreshTask(int tokenIndex) {
        if (m_uiOpMonitor && m_refreshTaskId[tokenIndex] > 0) {
            m_uiOpMonitor->failTask(m_refreshTaskId[tokenIndex], "Cancelled");
            m_refreshTaskId[tokenIndex] = 0;
        }
    }

    void SurfaceProfileService::cancelCalculation() {
        abortPendingRefresh(true);
        abortPendingRefresh(false);
        cancelPendingRefreshTask(0);
        cancelPendingRefreshTask(1);
        m_nominalCalculator.cancel();
        m_tolerancedCalculator.cancel();
        m_nominalCalcSlot = -1;
        m_tolerancedCalcSlot = -1;
    }

    void SurfaceProfileService::cancelCalculation(app::models::TaskSource source) {
        const bool nominal = (source == app::models::TaskSource::NominalSurfaceProfile);
        auto& calc = nominal ? m_nominalCalculator : m_tolerancedCalculator;
        int& slot = nominal ? m_nominalCalcSlot : m_tolerancedCalcSlot;
        abortPendingRefresh(nominal);
        cancelPendingRefreshTask(nominal ? 0 : 1);
        calc.cancel();
        slot = -1;
    }

    bool SurfaceProfileService::cancelTask(uint64_t taskId) {
        if (taskId == 0) return false;
        // A task created for the refresh phase has not reached the calculator yet.
        for (int i = 0; i < 2; ++i) {
            if (m_refreshTaskId[i] == taskId) {
                cancelCalculation(i == 0 ? app::models::TaskSource::NominalSurfaceProfile
                                         : app::models::TaskSource::TolerancedSurfaceProfile);
                return true;
            }
        }
        if (m_nominalCalculator.taskId() == taskId && m_nominalCalculator.isCalculating()) {
            cancelCalculation(app::models::TaskSource::NominalSurfaceProfile);
            return true;
        }
        if (m_tolerancedCalculator.taskId() == taskId && m_tolerancedCalculator.isCalculating()) {
            cancelCalculation(app::models::TaskSource::TolerancedSurfaceProfile);
            return true;
        }
        return false;
    }
}
