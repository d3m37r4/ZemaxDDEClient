#pragma once

#include <functional>
#include <string>
#include <vector>

#include "dde/dde_connection_manager.h"
#include "app/models/types.h"
#include "app/compute/surface_profile.h"
#include "app/services/operation_monitor_service.h"

class Logger;

namespace app::services {

    struct ProfileWindowState {
        int tolerancedSurfaceIndex = 0;
        int nominalSurfaceIndex = 0;

        int tolerancedSampling = 65;
        double tolerancedAngle = 0.0;
        int nominalSampling = 65;
        double nominalAngle = 0.0;
    };

    std::pair<std::vector<double>, std::vector<double>> extractSagCoordinates(const app::models::SurfaceData& surface);

    class SurfaceProfileService {
        public:
            SurfaceProfileService(DDEConnectionManager* connectionManager, Logger& logger);

            void setUiOperationMonitor(app::services::OperationMonitorService* monitor);

            void startCalculation(int surface, int sampling, double angle, app::models::TaskSource source = app::models::TaskSource::None);
            void cancelCalculation();

            bool isCalculating() const { return m_calculator.isCalculating(); }
            const app::models::SurfaceData& getResult() const;

            bool m_showTolerancedProfileWindow{false};
            bool m_showNominalProfileWindow{false};
            bool m_showComparisonProfileWindow{false};
            bool m_showDeviationProfileWindow{false};

            ProfileWindowState m_windowState;

            std::function<void()> onCalculationComplete;

            app::models::SurfaceData m_nominalSurfaceData;
            app::models::SurfaceData m_tolerancedSurfaceData;

        protected:
            void onCalculatorComplete();
            void onCalculatorFailed();

            DDEConnectionManager* m_connectionManager;
            Logger& m_logger;
            app::compute::SurfaceProfile m_calculator;

        private:
            app::models::TaskSource m_taskSource{app::models::TaskSource::None};
    };
}
