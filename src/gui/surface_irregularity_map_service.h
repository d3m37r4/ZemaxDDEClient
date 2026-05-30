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

            // Map calculation (own logic)
            void startMapCalculation(int surface, int numRadii, double angleStepDeg);
            void cancelMapCalculation();

            bool hasData() const { return !m_sections.empty(); }
            void clearData() { m_sections.clear(); }

            const std::vector<ZemaxDDE::SurfaceData>& getSections() const { return m_sections; }

            void renderSurfaceMapPlot(const ImVec2& size);
            bool m_showTolerancedSurfaceMap{false};

            const std::string& getMapError() const { return m_mapError; }

            MapWindowState m_windowState;
            ZemaxDDE::SurfaceData m_nominalSurfaceData;

        private:
            ZemaxDDE::ZemaxDDEClient* getClient() const { return m_connectionManager ? m_connectionManager->getActiveClient() : nullptr; }
            void sendNextSagPoint();
            void onMapSurfaceDataReceived(int code, const std::string& value);
            void onSagPointReceived(const std::string& buffer);
            void onSagTimeout();
            void onMapError(const std::string& error);

            DDEConnectionManager* m_connectionManager;
            Logger& m_logger;
            UiOperationMonitor* m_uiOpMonitor{nullptr};
            SurfaceProfileCalculator m_calculator;

            std::vector<ZemaxDDE::SurfaceData> m_sections;

            std::string m_mapError;

            int m_targetSurface = 0;
            int m_numRadii = 0;
            double m_angleStepDeg = 0.0;
            int m_numAngles = 0;
            double m_semiDiameter = 0.0;
            int m_units = 0;
            int m_surfaceRequestsRemaining = 0;
            uint64_t m_mapTaskId{0};
            int m_skippedPoints = 0;

            int m_currentRing = 0;
            int m_currentAngle = 0;
            ZemaxDDE::SurfaceData m_currentRingData;
    };
}
