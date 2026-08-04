#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "dde/dde_connection_manager.h"
#include "dde/client.h"
#include "app/models/types.h"
#include "app/services/operation_monitor_service.h"
#include "app/compute/surface_profile.h"
#include "app/compute/surface_map.h"

class Logger;

namespace app::services {

    // Guard rails that cap the DDE request load for a surface map so a pathological
    // UI value cannot spawn an unbounded number of sections/points (~millions).
    constexpr int    MAX_SAMPLING       = 4096;
    constexpr double MIN_ANGLE_STEP_DEG = 0.05;

    struct MapWindowState {
        int nominalSurfaceIndex = 0;
        int nominalSampling = 65;
        double nominalAngle = 0.0;

        int tolerancedSurfaceIndex = 0;
        int tolerancedSampling = 65;
        double tolerancedAngleStep = 1.0;

        int selectedColormapSurface = 0;
        int selectedColormapDeviation = 0;

        bool highlightWorstSurface = true;
        bool highlightWorstDeviation = true;
        float worstColorSurface[3] = {1.0f, 0.0f, 0.0f};
        float worstColorDeviation[3] = {1.0f, 0.0f, 0.0f};
    };

    using app::models::MaxPVResult;

    class SurfaceMapService {
        public:
            SurfaceMapService(DDEConnectionManager* connectionManager, Logger& logger);

            void setUiOperationMonitor(app::services::OperationMonitorService* monitor);

            void startCalculation(int surface, int sampling, double angle, app::models::TaskSource source);
            void cancelCalculation();
            std::function<void()> onNominalCalculationComplete;

            void startMapCalculation(int surface, int sampling, double angleStepDeg);
            void cancelMapCalculation();
            void clearData();

            /// Called when a connection slot is torn down. Cancels any calculation
            /// bound to that slot so its resources are released safely.
            void onClientDisconnected(int index);

            bool hasData() const { return !m_profiles.empty() && m_totalAngles > 0 && static_cast<int>(m_profiles.size()) >= m_totalAngles; }
            const std::vector<app::models::SurfaceData>& getProfiles() const { return m_profiles; }
            const std::optional<MaxPVResult>& getMaxPVResult() const { return m_maxPVResult; }
            std::optional<MaxPVResult> findMaxPVSection() const;
            bool calculateDeviations();

            bool m_showTolerancedSurfaceMap{false};
            bool m_showDeviationSurfaceMap{false};
            bool m_showWorstSectionProfile{false};
            bool m_showWorstSectionDeviation{false};
            app::models::SurfaceData m_worstProfileData;

            MapWindowState m_windowState;
            app::models::SurfaceData m_nominalSurfaceData;

            int getTargetSampling() const { return m_targetSampling; }
            int getTotalAngles() const { return m_totalAngles; }
            double getAngleStepDeg() const { return m_angleStepDeg; }
            int getTotalDdeRequests() const { return m_totalDdeRequests; }
            int getNominalCalcSlot() const { return m_nominalCalcSlot; }
            int getMapCalcSlot() const { return m_mapCalcSlot; }

        protected:
            void startNextProfile();

            DDEConnectionManager* m_connectionManager;
            Logger& m_logger;
            app::services::OperationMonitorService* m_uiOpMonitor{nullptr};
            app::compute::SurfaceProfile m_nominalCalculator;
            app::compute::SurfaceProfile m_mapCalculator;
            app::compute::SurfaceMap m_mapEngine;

            std::vector<app::models::SurfaceData> m_profiles;
            std::optional<MaxPVResult> m_maxPVResult;

            int m_targetSurface = 0;
            int m_targetSampling = 0;
            double m_angleStepDeg = 0.0;
            int m_totalAngles = 0;
            int m_currentAngleIndex = 0;
            double m_centerSagRef = 0.0;
            int m_totalDdeRequests{0};
            uint64_t m_mapTaskId{0};
            int m_nominalCalcSlot{-1};
            int m_mapCalcSlot{-1};
            std::chrono::steady_clock::time_point m_calcStartTime;
    };
}
