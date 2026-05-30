#pragma once

#include <functional>
#include <filesystem>
#include <string>
#include <vector>

#include "dde/dde_connection_manager.h"
#include "dde/client.h"
#include "gui/ui_operation_monitor.h"
#include "gui/surface_profile_calculator.h"

class Logger;

namespace gui {

    struct ProfileWindowState {
        int tolerancedSurfaceIndex = 0;
        int nominalSurfaceIndex = 0;

        int tolerancedSampling = 65;
        double tolerancedAngle = 0.0;
        int nominalSampling = 65;
        double nominalAngle = 0.0;
    };

    std::pair<std::vector<double>, std::vector<double>> extractSagCoordinates(const ZemaxDDE::SurfaceData& surface);

    std::optional<std::filesystem::path> writeToTemporaryFile(const std::string& filename, const std::string& content);

    class SurfaceProfileService {
        public:
            SurfaceProfileService(DDEConnectionManager* connectionManager, Logger& logger);

            void setUiOperationMonitor(UiOperationMonitor* monitor);
            void startCalculation(int surface, int sampling, double angle, TaskSource source = TaskSource::None);
            void saveCrossSectionToFile(const ZemaxDDE::SurfaceData& surface);

            void renderSurfaceProfilePlot(const char* plotLabel, const ZemaxDDE::SurfaceData& surface, const ImVec2& size);
            void renderProfileComparisonPlot(const char* plotLabel, const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, const ImVec2& size);
            void renderProfileDeviationPlot(const char* plotLabel, const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, const ImVec2& size);

            bool m_showTolerancedProfileWindow{false};
            bool m_showNominalProfileWindow{false};
            bool m_showComparisonProfileWindow{false};
            bool m_showDeviationProfileWindow{false};

            ProfileWindowState m_windowState;

            const ZemaxDDE::SurfaceData& getResult() const;

            void cancelCalculation();

            std::function<void()> onCalculationComplete;

            ZemaxDDE::SurfaceData m_nominalSurfaceData;
            ZemaxDDE::SurfaceData m_tolerancedSurfaceData;

        private:
            void onCalculatorComplete();
            void onCalculatorFailed();

            DDEConnectionManager* m_connectionManager;
            Logger& m_logger;
            SurfaceProfileCalculator m_calculator;
            TaskSource m_taskSource{TaskSource::None};
    };
}
