#include <cmath>
#include <format>

#include "dde/constants.h"
#include "dde/utils.h"

#include "app/services/surface_profile_service.h"
#include "logger/logger.h"

namespace app::services {
    SurfaceProfileService::SurfaceProfileService(DDEConnectionManager* connectionManager, Logger& logger)
        : m_connectionManager(connectionManager)
        , m_logger(logger)
        , m_calculator(connectionManager, logger)
    {
    }

    void SurfaceProfileService::setUiOperationMonitor(app::services::OperationMonitor* monitor) {
        m_calculator.setMonitor(monitor);
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

    void SurfaceProfileService::startCalculation(int surface, int sampling, double angle, app::models::TaskSource source) {
        m_taskSource = source;

        if (m_connectionManager) {
            m_calculator.setSurfaceDataTimeoutMs(m_connectionManager->getGetSurfaceDataProfileTimeoutMs());
            m_calculator.setSagTimeoutMs(m_connectionManager->getGetSagProfileTimeoutMs());
        }

        m_calculator.onComplete = [this]() { onCalculatorComplete(); };
        m_calculator.onFailed = [this]() { onCalculatorFailed(); };

        m_calculator.startCalculation(surface, sampling, angle, source);
    }

    void SurfaceProfileService::onCalculatorComplete() {
        const auto& result = m_calculator.getResult();

        if (m_taskSource == app::models::TaskSource::NominalSurfaceProfile) {
            m_nominalSurfaceData = result;
        } else {
            m_tolerancedSurfaceData = result;
        }

        if (onCalculationComplete) onCalculationComplete();
    }

    void SurfaceProfileService::onCalculatorFailed() {
        if (onCalculationComplete) onCalculationComplete();
    }

    void SurfaceProfileService::cancelCalculation() {
        m_calculator.cancel();
    }

    const app::models::SurfaceData& SurfaceProfileService::getResult() const {
        return m_calculator.getResult();
    }
}
