#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "dde/dde_connection_manager.h"
#include "dde/client.h"

class Logger;

namespace gui {

    enum class SagCalcState {
        Idle,
        FetchingSurfaceData,
        FetchingSagPoints,
        Completed,
        Failed
    };

    struct SagAnalysisState {
        int tolerancedSurfaceIndex = 0;
        int nominalSurfaceIndex = 0;

        int tolerancedSampling = 128;
        double tolerancedAngle = 0.0;
        int nominalSampling = 128;
        double nominalAngle = 0.0;
    };

    std::pair<std::vector<double>, std::vector<double>> extractSagCoordinates(const ZemaxDDE::SurfaceData& surface);

    std::optional<std::filesystem::path> writeToTemporaryFile(const std::string& filename, const std::string& content);

    class SurfaceProfileService {
        public:
            SurfaceProfileService(DDEConnectionManager* connectionManager, Logger& logger);

            void startAsyncSagCalculation(int surface, int sampling, double angle);
            void saveCrossSectionToFile(const ZemaxDDE::SurfaceData& surface);

            void renderSurfaceProfilePlot(const char* plotLabel, const ZemaxDDE::SurfaceData& surface, const ImVec2& size);
            void renderProfileComparisonPlot(const char* plotLabel, const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, const ImVec2& size);
            void renderProfileDeviationPlot(const char* plotLabel, const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, const ImVec2& size);

            bool m_showTolerancedProfileWindow{false};
            bool m_showNominalProfileWindow{false};
            bool m_showComparisonProfileWindow{false};
            bool m_showDeviationProfileWindow{false};

            SagAnalysisState m_pageState;

            SagCalcState getCalcState() const { return m_calcState; }
            const std::string& getCalcError() const { return m_calcError; }
            const ZemaxDDE::SurfaceData& getResult() const { return m_resultSurface; }

            uint64_t getOperationId() const { return m_operationId; }
            int getTotalSteps() const { return m_targetSampling; }
            int getCurrentStep() const { return m_sagPointIndex; }
            int getSkippedPoints() const { return m_skippedPoints; }

            void cancelCalculation();

            std::function<void()> onCalculationComplete;

            ZemaxDDE::SurfaceData m_nominalSurfaceData;
            ZemaxDDE::SurfaceData m_tolerancedSurfaceData;

        private:
            ZemaxDDE::ZemaxDDEClient* getClient() const;
            ZemaxDDE::OperationMonitor* getMonitor() const;
            void sendNextSagRequest();
            void onSurfaceDataReceived(int code, const std::string& value);
            void onSagDataReceived(const std::string& buffer);
            void onError(const std::string& error);
            void onSagTimeout();

            DDEConnectionManager* m_connectionManager;
            Logger& m_logger;

            SagCalcState m_calcState = SagCalcState::Idle;
            std::string m_calcError;

            int m_targetSurface = 0;
            int m_targetSampling = 0;
            double m_targetAngle = 0.0;
            int m_sagPointIndex = 0;
            int m_surfaceRequestsRemaining = 0;
            uint64_t m_operationId = 0;
            int m_skippedPoints = 0;
            std::chrono::steady_clock::time_point m_calcStartTime;

            ZemaxDDE::SurfaceData m_resultSurface;
    };
}
