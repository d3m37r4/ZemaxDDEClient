#include <cmath>
#include <format>

#include "dde/utils.h"

#include "app/services/surface_profile_service.h"
#include "logger/logger.h"

namespace app::services {
    SurfaceProfileService::SurfaceProfileService(DDEConnectionManager* connectionManager, Logger& logger)
        : m_connectionManager(connectionManager)
        , m_logger(logger)
        , m_nominalCalculator(connectionManager, logger)
        , m_tolerancedCalculator(connectionManager, logger)
    {
    }

    void SurfaceProfileService::setUiOperationMonitor(app::services::OperationMonitorService* monitor) {
        m_nominalCalculator.setMonitor(monitor);
        m_tolerancedCalculator.setMonitor(monitor);
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

    const app::models::SurfaceData& SurfaceProfileService::getResult() const {
        return m_nominalCalculator.getResult();
    }

    const app::models::SurfaceData& SurfaceProfileService::getResult(app::models::TaskSource source) const {
        if (source == app::models::TaskSource::NominalSurfaceProfile)
            return m_nominalCalculator.getResult();
        return m_tolerancedCalculator.getResult();
    }

    void SurfaceProfileService::startCalculation(int surface, int sampling, double angle, app::models::TaskSource source) {
        auto& calc = (source == app::models::TaskSource::NominalSurfaceProfile)
            ? m_nominalCalculator : m_tolerancedCalculator;
        int& slot = (source == app::models::TaskSource::NominalSurfaceProfile)
            ? m_nominalCalcSlot : m_tolerancedCalcSlot;

        slot = m_connectionManager ? m_connectionManager->getActiveIndex() : -1;

        if (m_connectionManager) {
            calc.setSurfaceDataTimeoutMs(m_connectionManager->getGetSurfaceDataProfileTimeoutMs());
            calc.setSagTimeoutMs(m_connectionManager->getGetSagProfileTimeoutMs());
        }

        if (source == app::models::TaskSource::NominalSurfaceProfile) {
            calc.onComplete = [this]() { onNominalComplete(); };
            calc.onFailed = [this]() { onNominalFailed(); };
        } else {
            calc.onComplete = [this]() { onTolerancedComplete(); };
            calc.onFailed = [this]() { onTolerancedFailed(); };
        }

        calc.startCalculation(surface, sampling, angle, source);
    }

    void SurfaceProfileService::onNominalComplete() {
        const auto& result = m_nominalCalculator.getResult();
        m_nominalSurfaceData = result;
        m_nominalCalcSlot = -1;
        if (onNominalCalculationComplete) onNominalCalculationComplete();
    }

    void SurfaceProfileService::onNominalFailed() {
        m_nominalCalcSlot = -1;
        if (onNominalCalculationComplete) onNominalCalculationComplete();
    }

    void SurfaceProfileService::onTolerancedComplete() {
        const auto& result = m_tolerancedCalculator.getResult();
        m_tolerancedSurfaceData = result;
        m_tolerancedCalcSlot = -1;
        if (onTolerancedCalculationComplete) onTolerancedCalculationComplete();
    }

    void SurfaceProfileService::onTolerancedFailed() {
        m_tolerancedCalcSlot = -1;
        if (onTolerancedCalculationComplete) onTolerancedCalculationComplete();
    }

    void SurfaceProfileService::cancelCalculation() {
        m_nominalCalculator.cancel();
        m_tolerancedCalculator.cancel();
        m_nominalCalcSlot = -1;
        m_tolerancedCalcSlot = -1;
    }

    void SurfaceProfileService::cancelCalculation(app::models::TaskSource source) {
        auto& calc = (source == app::models::TaskSource::NominalSurfaceProfile)
            ? m_nominalCalculator : m_tolerancedCalculator;
        int& slot = (source == app::models::TaskSource::NominalSurfaceProfile)
            ? m_nominalCalcSlot : m_tolerancedCalcSlot;
        calc.cancel();
        slot = -1;
    }
}
