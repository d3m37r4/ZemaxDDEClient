#include <algorithm>
#include <cmath>
#include <format>
#include <ranges>

#include "sag_map_analysis_service.h"
#include "logger/logger.h"

namespace {
    constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;
}

namespace gui {
    SagMapAnalysisService::SagMapAnalysisService(ZemaxDDE::ZemaxDDEClient* ddeClient, Logger& logger)
        : m_ddeClient(ddeClient)
        , m_logger(logger)
    {
    }

    void SagMapAnalysisService::calculateSurfaceMap(int surface, int numRadii, double angleStepDeg) {
        if (angleStepDeg <= 0.0 || angleStepDeg > 360.0) {
            m_logger.addLog("[SagMap] Invalid angle step, must be in range (0, 360]");
            return;
        }

        if (numRadii < 2) {
            m_logger.addLog("[SagMap] Invalid number of radii, must be >= 2");
            return;
        }

        clearData();

        m_logger.addLog(std::format("[SagMap] Calculating surface map for surface {}, radii={}, angle step={} deg",
            surface, numRadii, angleStepDeg));

        auto* toleranced = m_ddeClient->getTolerancedSurface();
        if (!toleranced->isValid() || toleranced->semiDiameter <= 0.0) {
            m_logger.addLog("[SagMap] Toleranced surface not initialized or invalid");
            return;
        }

        double semiDiameter = toleranced->semiDiameter;
        double radiusStep = semiDiameter / (numRadii - 1);
        int numAngles = static_cast<int>(360.0 / angleStepDeg);

        for (int i = 0; i < numRadii; ++i) {
            double r = i * radiusStep;

            ZemaxDDE::SurfaceData ring;
            ring.id = surface;
            ring.semiDiameter = semiDiameter;
            ring.units = toleranced->units;
            ring.type = toleranced->type;
            ring.fileName = toleranced->fileName;

            for (int j = 0; j < numAngles; ++j) {
                double angle = j * angleStepDeg;
                double rad = angle * DEG_TO_RAD;
                double x = r * std::cos(rad);
                double y = r * std::sin(rad);

                toleranced->sagDataPoints.clear();
                m_ddeClient->getSag(surface, x, y);

                if (!toleranced->sagDataPoints.empty()) {
                    ring.sagDataPoints.push_back(toleranced->sagDataPoints.back());
                }
            }

            m_sections.push_back(ring);
        }

        m_state.tolerancedSurfaceIndex = surface;
        m_state.tolerancedSampling = numRadii;
        m_state.tolerancedAngleStep = angleStepDeg;

        m_logger.addLog(std::format("[SagMap] Surface map calculated: {} rings", m_sections.size()));
    }

    // TODO: Re-enable Max-PV analysis
    /*
    std::optional<MaxPVResult> SagMapAnalysisService::findMaxPVSection() const {
        if (m_sections.empty() || !hasNominalReference()) {
            m_logger.addLog("[SagMap] No data available for Max-PV search");
            return std::nullopt;
        }

        const auto* nominal = m_ddeClient->getNominalSurface();
        if (nominal->sagDataPoints.empty()) {
            m_logger.addLog("[SagMap] Nominal reference has no sag data");
            return std::nullopt;
        }

        MaxPVResult worst;
        worst.pv = 0.0;
        worst.angle = 0.0;
        worst.peak = 0.0;
        worst.valley = 0.0;

        for (const auto& section : m_sections) {
            if (section.sagDataPoints.size() != nominal->sagDataPoints.size()) {
                continue;
            }

            double localPeak = -1e100;
            double localValley = 1e100;

            for (size_t i = 0; i < section.sagDataPoints.size(); ++i) {
                double deviation = section.sagDataPoints[i].sag - nominal->sagDataPoints[i].sag;
                localPeak = std::max(localPeak, deviation);
                localValley = std::min(localValley, deviation);
            }

            double pv = localPeak - localValley;
            if (pv > worst.pv) {
                worst.angle = section.angle;
                worst.peak = localPeak;
                worst.valley = localValley;
                worst.pv = pv;
            }
        }

        m_logger.addLog(std::format("[SagMap] Max-PV found at {:.2f} deg: Peak={:.6f}, Valley={:.6f}, PV={:.6f}",
            worst.angle, worst.peak, worst.valley, worst.pv));

        return worst;
    }
    */
}
