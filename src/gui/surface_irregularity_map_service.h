#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "dde/dde_connection_manager.h"
#include "dde/client.h"
#include "gui/ui_operation_monitor.h"
#include "gui/surface_profile_calculator.h"
#include "lib/imgui/imgui.h"

class Logger;

namespace ZemaxDDE {
    class OperationMonitor;
}

namespace gui {

    struct MapWindowState {
        int nominalSurfaceIndex = 0;
        int nominalSampling = 65;
        double nominalAngle = 0.0;

        int tolerancedSurfaceIndex = 0;
        int tolerancedSampling = 65;
        double tolerancedAngleStep = 1.0;
    };

    struct MaxPVResult {
        double angle;
        double peak;
        double valley;
        double pv;
    };

    class SurfaceIrregularityMapService {
        public:
            SurfaceIrregularityMapService(DDEConnectionManager* connectionManager, Logger& logger);

            void setUiOperationMonitor(UiOperationMonitor* monitor);

            // Profile calculation (via SurfaceProfileCalculator)
            void startCalculation(int surface, int sampling, double angle, TaskSource source);
            void cancelCalculation();
            std::function<void()> onCalculationComplete;

            // Map calculation (via profile sections)
            void startMapCalculation(int surface, int sampling, double angleStepDeg);
            void cancelMapCalculation();
            void clearData();

            bool hasData() const { return !m_profiles.empty() && m_totalAngles > 0 && static_cast<int>(m_profiles.size()) >= m_totalAngles; }
            const std::vector<ZemaxDDE::SurfaceData>& getProfiles() const { return m_profiles; }
            const std::optional<MaxPVResult>& getMaxPVResult() const { return m_maxPVResult; }
            std::optional<MaxPVResult> findMaxPVSection() const;

            void renderSurfaceMapPlot(const ImVec2& size);
            void renderDeviationSurfaceMapPlot(const ImVec2& size);
            bool m_showTolerancedSurfaceMap{false};
            bool m_showDeviationSurfaceMap{false};
            bool m_showWorstSectionProfile{false};
            bool m_showWorstSectionDeviation{false};
            ZemaxDDE::SurfaceData m_worstProfileData;

            MapWindowState m_windowState;
            ZemaxDDE::SurfaceData m_nominalSurfaceData;

        private:
            struct SurfaceMatrices {
                std::vector<float> X, Y, Z;
                float zMin{0}, zMax{0};
            };

            SurfaceMatrices buildSurfaceMatrices(bool deviation) const;
            void startNextProfile();

            ZemaxDDE::ZemaxDDEClient* getClient() const { return m_connectionManager ? m_connectionManager->getActiveClient() : nullptr; }

            DDEConnectionManager* m_connectionManager;
            Logger& m_logger;
            UiOperationMonitor* m_uiOpMonitor{nullptr};
            SurfaceProfileCalculator m_calculator;

            std::vector<ZemaxDDE::SurfaceData> m_profiles;
            std::optional<MaxPVResult> m_maxPVResult;

            int m_targetSurface = 0;
            int m_targetSampling = 0;
            double m_angleStepDeg = 0.0;
            int m_totalAngles = 0;
            int m_currentAngleIndex = 0;
            double m_centerSagRef = 0.0;
            uint64_t m_mapTaskId{0};
    };
}
