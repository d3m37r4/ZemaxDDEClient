#pragma once

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

    class SagAnalysisService {
        public:
            SagAnalysisService(DDEConnectionManager* connectionManager, Logger& logger);

            void startAsyncSagCalculation(int surface, int sampling, double angle);
            void saveCrossSectionToFile(const ZemaxDDE::SurfaceData& surface);

            void renderCrossSectionWindow(const char* title, const char* label, const ZemaxDDE::SurfaceData& surface, bool* openFlag);
            void renderComparisonWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag);
            void renderErrorWindow(const ZemaxDDE::SurfaceData& nominal, const ZemaxDDE::SurfaceData& toleranced, bool* openFlag);

            bool m_showTolerancedSagWindow{false};
            bool m_showNominalSagWindow{false};
            bool m_showComparisonWindow{false};
            bool m_showErrorWindow{false};

            SagAnalysisState m_surfaceSagAnalysisPageState;

            SagCalcState getCalcState() const { return m_calcState; }
            const std::string& getCalcError() const { return m_calcError; }
            const ZemaxDDE::SurfaceData& getResult() const { return m_resultSurface; }

            uint64_t getOperationId() const { return m_operationId; }
            int getTotalSteps() const { return m_targetSampling; }
            int getCurrentStep() const { return m_sagPointIndex; }
            int getSkippedPoints() const { return m_skippedPoints; }

            void cancelCalculation();

            std::function<void()> onCalculationComplete;

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

            ZemaxDDE::SurfaceData m_resultSurface;
    };
}
