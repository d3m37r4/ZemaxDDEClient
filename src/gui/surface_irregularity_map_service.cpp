#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>

#include "dde/constants.h"
#include "dde/operation_monitor.h"
#include "dde/utils.h"

#include "surface_irregularity_map_service.h"
#include "logger/logger.h"
#include "lib/implot3d/implot3d.h"

namespace {
    constexpr double DEG_TO_RAD = std::numbers::pi / 180.0;
}

namespace gui {
    SurfaceIrregularityMapService::SurfaceIrregularityMapService(DDEConnectionManager* connectionManager, Logger& logger)
        : m_connectionManager(connectionManager)
        , m_logger(logger)
        , m_calculator(connectionManager, logger)
    {
    }

    void SurfaceIrregularityMapService::setUiOperationMonitor(UiOperationMonitor* monitor) {
        m_uiOpMonitor = monitor;
        m_calculator.setMonitor(monitor);
    }

    // --- Profile calculation (via SurfaceProfileCalculator) ---

    void SurfaceIrregularityMapService::startCalculation(int surface, int sampling, double angle, TaskSource source) {
        // Set per-map timeout overrides from DDEConnectionManager.
        if (m_connectionManager) {
            m_calculator.setSurfaceDataTimeoutMs(m_connectionManager->getGetSurfaceDataMapTimeoutMs());
            m_calculator.setSagTimeoutMs(m_connectionManager->getGetSagMapTimeoutMs());
        }

        m_calculator.onComplete = [this]() {
            m_nominalSurfaceData = m_calculator.getResult();
            if (m_nominalSurfaceData.isValid()) {
                m_logger.addLog("[IrregularityMapService] Nominal reference set");
            }
            if (onCalculationComplete) onCalculationComplete();
        };
        m_calculator.onFailed = [this]() {
            if (onCalculationComplete) onCalculationComplete();
        };

        m_calculator.startCalculation(surface, sampling, angle, source);
    }

    void SurfaceIrregularityMapService::cancelCalculation() {
        m_calculator.cancel();
    }

    // --- Map calculation (via profile sections) ---

    void SurfaceIrregularityMapService::startMapCalculation(int surface, int sampling, double angleStepDeg) {
        auto* client = getClient();
        if (!client) {
            m_logger.addLog("[IrregularityMapService] No active DDE connection");
            return;
        }

        // Set per-map timeout overrides from DDEConnectionManager.
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

        m_logger.addLog(std::format("[IrregularityMapService] Starting surface map: surface {}, {} sections ({}° step, {} pts each)",
            surface, m_totalAngles, angleStepDeg, sampling));

        if (m_uiOpMonitor) {
            m_mapTaskId = m_uiOpMonitor->startTask(
                TaskSource::SurfaceIrregularityMap, "Surface Irregularity Map", m_totalAngles);
        }

        startNextProfile();
    }

    void SurfaceIrregularityMapService::startNextProfile() {
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

            m_logger.addLog(std::format("[IrregularityMapService] Surface map completed: {} sections", m_profiles.size()));

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
            if (m_calculator.isCancelled()) {
                m_logger.addLog("[IrregularityMapService] Surface map cancelled");
            } else {
                m_logger.addLog(std::format("[IrregularityMapService] Profile calculation failed: {}", m_calculator.getError()));
            }
            if (m_uiOpMonitor && m_mapTaskId > 0) {
                m_uiOpMonitor->failTask(m_mapTaskId, m_calculator.getError().empty() ? "Cancelled" : m_calculator.getError());
                m_mapTaskId = 0;
            }
            m_profiles.clear();
            m_currentAngleIndex = 0;
        };

        m_calculator.startCalculation(m_targetSurface, m_targetSampling, angle,
            TaskSource::SurfaceIrregularityMap,
            std::format("Section {}/{} at {:.1f}°", m_currentAngleIndex + 1, m_totalAngles, angle));
    }

    void SurfaceIrregularityMapService::cancelMapCalculation() {
        if (m_calculator.isCalculating()) {
            m_calculator.cancel();
        } else if (m_mapTaskId > 0) {
            if (m_uiOpMonitor) m_uiOpMonitor->failTask(m_mapTaskId, "Cancelled");
            m_mapTaskId = 0;
            m_profiles.clear();
            m_currentAngleIndex = 0;
        }
    }

    void SurfaceIrregularityMapService::clearData() {
        m_profiles.clear();
        m_maxPVResult.reset();
        m_currentAngleIndex = 0;
        m_mapTaskId = 0;
    }

    std::optional<MaxPVResult> SurfaceIrregularityMapService::findMaxPVSection() const {
        if (m_profiles.empty() || !m_nominalSurfaceData.isValid()) return std::nullopt;
        if (m_profiles[0].sagDataPoints.size() != m_nominalSurfaceData.sagDataPoints.size()) return std::nullopt;

        std::optional<MaxPVResult> best;
        size_t numPoints = m_nominalSurfaceData.sagDataPoints.size();

        for (const auto& profile : m_profiles) {
            if (profile.sagDataPoints.size() != numPoints) continue;

            double P = -1e30, V = 1e30;
            for (size_t i = 0; i < numPoints; ++i) {
                double delta = profile.sagDataPoints[i].sag - m_nominalSurfaceData.sagDataPoints[i].sag;
                if (delta > P) P = delta;
                if (delta < V) V = delta;
            }
            double pv = P - V;

            if (!best.has_value() || pv > best->pv) {
                best = MaxPVResult{profile.angle, P, V, pv};
            }
        }

        return best;
    }

    SurfaceIrregularityMapService::SurfaceMatrices SurfaceIrregularityMapService::buildSurfaceMatrices(bool deviation) const {
        SurfaceMatrices result;
        if (m_profiles.empty()) return result;

        int numRadii = (m_targetSampling + 1) / 2;
        int numAngles = m_totalAngles * 2;
        int centerIdx = (m_targetSampling - 1) / 2;
        double semiDiameter = m_profiles[0].semiDiameter;

        result.X.resize(static_cast<size_t>(numRadii * numAngles));
        result.Y.resize(static_cast<size_t>(numRadii * numAngles));
        result.Z.resize(static_cast<size_t>(numRadii * numAngles));
        bool first = true;

        int numProfiles = static_cast<int>(m_profiles.size());

        for (int j = 0; j < numAngles; ++j) {
            int profileIdx = std::min(j / 2, numProfiles - 1);
            bool positiveHalf = (j % 2 == 0);
            double angleRad = profileIdx * m_angleStepDeg * DEG_TO_RAD;
            if (!positiveHalf) angleRad += std::numbers::pi;

            double rStep = semiDiameter / (numRadii - 1);

            for (int i = 0; i < numRadii; ++i) {
                double r = i * rStep;
                size_t idx = static_cast<size_t>(i * numAngles + j);
                result.X[idx] = static_cast<float>(r * std::cos(angleRad));
                result.Y[idx] = static_cast<float>(r * std::sin(angleRad));

                int srcIdx = positiveHalf ? centerIdx + i : centerIdx - i;
                srcIdx = std::max(0, std::min(static_cast<int>(m_profiles[profileIdx].sagDataPoints.size()) - 1, srcIdx));

                float sag = static_cast<float>(m_profiles[profileIdx].sagDataPoints[srcIdx].sag);

                if (deviation && m_nominalSurfaceData.isValid() && srcIdx < static_cast<int>(m_nominalSurfaceData.sagDataPoints.size())) {
                    sag -= static_cast<float>(m_nominalSurfaceData.sagDataPoints[srcIdx].sag);
                }

                result.Z[idx] = sag;

                if (first) { result.zMin = sag; result.zMax = sag; first = false; }
                else { result.zMin = std::min(result.zMin, sag); result.zMax = std::max(result.zMax, sag); }
            }
        }

        return result;
    }

    void SurfaceIrregularityMapService::renderSurfaceMapPlot(const ImVec2& size) {
        auto matrices = buildSurfaceMatrices(false);
        if (matrices.X.empty()) return;

        int numRadii = (m_targetSampling + 1) / 2;
        int numAngles = m_totalAngles * 2;

        if (ImPlot3D::BeginPlot("##Surface3D", size)) {
            ImPlot3D::SetupAxes("X (mm)", "Y (mm)", "Z (mm)");
            ImPlot3D::PlotSurface("Lens Surface", matrices.X.data(), matrices.Y.data(), matrices.Z.data(), numRadii, numAngles, matrices.zMin, matrices.zMax);
            ImPlot3D::EndPlot();
        }
    }

    void SurfaceIrregularityMapService::renderDeviationSurfaceMapPlot(const ImVec2& size) {
        auto matrices = buildSurfaceMatrices(true);
        if (matrices.X.empty()) return;

        int numRadii = (m_targetSampling + 1) / 2;
        int numAngles = m_totalAngles * 2;

        if (ImPlot3D::BeginPlot("##Deviation3D", size)) {
            ImPlot3D::SetupAxes("X (mm)", "Y (mm)", "ΔSag (mm)");
            ImPlot3D::PlotSurface("Deviation", matrices.X.data(), matrices.Y.data(), matrices.Z.data(), numRadii, numAngles, matrices.zMin, matrices.zMax);
            ImPlot3D::EndPlot();
        }
    }
}
