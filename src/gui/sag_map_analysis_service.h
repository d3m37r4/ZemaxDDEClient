#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "dde/dde_connection_manager.h"
#include "dde/client.h"
#include "gui/ui_operation_monitor.h"

class Logger;

namespace ZemaxDDE {
    class OperationMonitor;
}

namespace gui {

    enum class SagMapState {
        Idle,
        FetchingSurfaceData,
        FetchingSagPoints,
        Completed,
        Failed
    };

    struct SagMapAnalysisState {
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

    class SagMapAnalysisService {
        public:
            SagMapAnalysisService(DDEConnectionManager* connectionManager, Logger& logger);

            void setUiOperationMonitor(UiOperationMonitor* monitor) { m_uiOpMonitor = monitor; }

            void startAsyncMapCalculation(int surface, int numRadii, double angleStepDeg);

            bool hasNominalReference() const;
            const ZemaxDDE::SurfaceData& getNominalReference() const;

            bool hasData() const { return !m_sections.empty(); }
            void clearData() { m_sections.clear(); }

            const std::vector<ZemaxDDE::SurfaceData>& getSections() const { return m_sections; }

            SagMapState getMapState() const { return m_mapState; }
            const std::string& getMapError() const { return m_mapError; }

            uint64_t getOperationId() const { return m_operationId; }
            int getTotalSteps() const { return m_numRadii * m_numAngles; }
            int getCurrentStep() const { return m_currentRing * m_numAngles + m_currentAngle; }
            int getSkippedPoints() const { return m_skippedPoints; }

            void cancelCalculation();

            std::function<void()> onCalculationComplete;

            SagMapAnalysisState m_state;

        private:
            ZemaxDDE::ZemaxDDEClient* getClient() const { return m_connectionManager ? m_connectionManager->getActiveClient() : nullptr; }
            ZemaxDDE::OperationMonitor* getMonitor() const;
            void sendNextSagPoint();
            void onSurfaceDataReceived(int code, const std::string& value);
            void onSagPointReceived(const std::string& buffer);
            void onSagTimeout();
            void onMapError(const std::string& error);

            DDEConnectionManager* m_connectionManager;
            Logger& m_logger;
            UiOperationMonitor* m_uiOpMonitor{nullptr};

            std::vector<ZemaxDDE::SurfaceData> m_sections;

            SagMapState m_mapState = SagMapState::Idle;
            std::string m_mapError;

            int m_targetSurface = 0;
            int m_numRadii = 0;
            double m_angleStepDeg = 0.0;
            int m_numAngles = 0;
            double m_semiDiameter = 0.0;
            int m_units = 0;
            int m_surfaceRequestsRemaining = 0;
            uint64_t m_taskId{0};
            uint64_t m_operationId = 0;
            int m_skippedPoints = 0;

            int m_currentRing = 0;
            int m_currentAngle = 0;
            ZemaxDDE::SurfaceData m_currentRingData;
    };
}
