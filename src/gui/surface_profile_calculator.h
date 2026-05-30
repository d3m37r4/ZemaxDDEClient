#pragma once

#include <chrono>
#include <functional>
#include <string>

#include "dde/dde_connection_manager.h"
#include "dde/client.h"
#include "gui/ui_operation_monitor.h"

class Logger;

namespace gui {
    class SurfaceProfileCalculator {
        public:
            SurfaceProfileCalculator(DDEConnectionManager* connectionManager, Logger& logger);

            void setMonitor(UiOperationMonitor* monitor) { m_uiOpMonitor = monitor; }

            void startCalculation(int surface, int sampling, double angle, TaskSource source);
            void cancel();
            void reset() { m_state = State::Idle; m_error.clear(); m_result = {}; m_taskId = 0; m_skippedPoints = 0; }

            bool isCalculating() const { return m_state == State::FetchingSurfaceData || m_state == State::FetchingSagPoints; }
            bool isCancelled() const;
            const std::string& getError() const { return m_error; }
            const ZemaxDDE::SurfaceData& getResult() const { return m_result; }
            void setResultExtras(int units, const std::string& fileName) { m_result.units = units; m_result.fileName = fileName; }
            TaskSource getSource() const { return m_source; }

            std::function<void()> onComplete;
            std::function<void()> onFailed;

        private:
            ZemaxDDE::ZemaxDDEClient* getClient() const;
            void sendNextSagRequest();
            void onSurfaceDataReceived(int code, const std::string& value);
            void onSagDataReceived(const std::string& buffer);
            void onSagTimeout();
            void onError(const std::string& error);

            DDEConnectionManager* m_connectionManager;
            Logger& m_logger;
            UiOperationMonitor* m_uiOpMonitor{nullptr};

            enum class State { Idle, FetchingSurfaceData, FetchingSagPoints, Completed, Failed };
            State m_state = State::Idle;
            std::string m_error;

            TaskSource m_source{TaskSource::None};
            uint64_t m_taskId{0};
            int m_targetSurface = 0;
            int m_targetSampling = 0;
            double m_targetAngle = 0.0;
            int m_sagPointIndex = 0;
            int m_surfaceRequestsRemaining = 0;
            int m_skippedPoints = 0;
            std::chrono::steady_clock::time_point m_calcStartTime;

            ZemaxDDE::SurfaceData m_result;
    };
}
