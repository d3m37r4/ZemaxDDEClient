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

    void SagMapAnalysisService::calculateSurfaceMap(int surface, int radialSampling, double angleStepDeg) {
        if (angleStepDeg <= 0.0 || angleStepDeg > 180.0) {
            m_logger.addLog("[SagMap] Invalid angle step, must be in range (0, 180]");
            return;
        }

        clearData();

        m_logger.addLog(std::format("[SagMap] Calculating surface map for surface {}, sampling={}, angle step={} deg",
            surface, radialSampling, angleStepDeg));

        m_ddeClient->setStorageTarget(m_ddeClient->getNominalSurface());
        m_ddeClient->getSurfaceData(surface, ZemaxDDE::SurfaceDataCode::TYPE_NAME);
        m_ddeClient->getSurfaceData(surface, ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER);

        const ZemaxDDE::SurfaceData* nominalSurface = m_ddeClient->getNominalSurface();
        if (!nominalSurface->isValid()) {
            m_logger.addLog("[SagMap] Failed to get nominal surface data");
            return;
        }

        double semiDiameter = nominalSurface->semiDiameter;
        if (semiDiameter <= 0.0) {
            m_logger.addLog("[SagMap] Invalid semi-diameter");
            return;
        }

        double step = nominalSurface->diameter() / (radialSampling - 1);
        int numAngles = static_cast<int>(180.0 / angleStepDeg) + 1;

        m_ddeClient->setStorageTarget(m_ddeClient->getTolerancedSurface());

        for (int i = 0; i < numAngles; ++i) {
            double angle = i * angleStepDeg;
            double rad = angle * DEG_TO_RAD;
            double cosAngle = std::cos(rad);
            double sinAngle = std::sin(rad);

            m_ddeClient->getTolerancedSurface()->clear();
            m_ddeClient->getTolerancedSurface()->id = surface;
            m_ddeClient->getTolerancedSurface()->units = nominalSurface->units;
            m_ddeClient->getTolerancedSurface()->semiDiameter = semiDiameter;
            m_ddeClient->getTolerancedSurface()->type = nominalSurface->type;
            m_ddeClient->getTolerancedSurface()->fileName = nominalSurface->fileName;

            for (int j : std::views::iota(0, radialSampling)) {
                double r = -semiDiameter + j * step;
                double x = r * cosAngle;
                double y = r * sinAngle;
                m_ddeClient->getSag(surface, x, y);
            }

            m_sections.push_back(*m_ddeClient->getTolerancedSurface());
        }

        m_state.tolerancedSurfaceIndex = surface;
        m_state.tolerancedSampling = radialSampling;
        m_state.tolerancedAngleStep = angleStepDeg;

        m_logger.addLog(std::format("[SagMap] Surface map calculated: {} sections", m_sections.size()));
    }

    std::optional<MaxPVResult> SagMapAnalysisService::findMaxPVSection() const {
        if (m_sections.empty()) {
            m_logger.addLog("[SagMap] No data available for Max-PV search");
            return std::nullopt;
        }

        MaxPVResult worst;
        worst.pv = 0.0;

        for (const auto& section : m_sections) {
            if (section.sagDataPoints.empty()) {
                continue;
            }

            double localPeak = -1e100;
            double localValley = 1e100;

            for (const auto& point : section.sagDataPoints) {
                localPeak = std::max(localPeak, point.sag);
                localValley = std::min(localValley, point.sag);
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
}
