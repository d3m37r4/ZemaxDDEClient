#pragma once

#include <chrono>
#include <functional>
#include <string>

#include "app/models/types.h"
#include "app/models/surface_data.h"
#include "dde/dde_connection_manager.h"
#include "dde/client.h"

namespace app::services { class OperationMonitorService; }
class Logger;

namespace app::compute {

    class SurfaceProfile {
        public:
            SurfaceProfile(DDEConnectionManager* connectionManager, Logger& logger);

            void setMonitor(app::services::OperationMonitorService* monitor) { m_uiOpMonitor = monitor; }

            void setSurfaceDataTimeoutMs(DWORD ms) { m_surfaceDataTimeoutMsOverride = ms; }
            void setSagTimeoutMs(DWORD ms) { m_sagTimeoutMsOverride = ms; }

            void startCalculation(int surface, int sampling, double angle,
                                  app::models::TaskSource source, const std::string& label = "");
            void cancel();

            bool isCalculating() const { return m_state == State::FetchingSurfaceData || m_state == State::FetchingSagPoints; }
            bool isCancelled() const;
            const std::string& getError() const { return m_error; }
            const app::models::SurfaceData& getResult() const { return m_result; }
            int getTotalDdeRequests() const { return m_totalDdeRequests; }

            std::function<void()> onComplete;
            std::function<void()> onFailed;
            std::function<void(int currentStep, int totalSteps, const std::string& message)> onProgress;

            bool m_createTask{true};

        private:
            ZemaxDDE::ZemaxDDEClient* getClient() const;
            void sendNextSagRequest();
            void onSurfaceDataReceived(int code, const std::string& value);
            void onSagDataReceived(const std::string& buffer);
            void onSagTimeout();
            void onError(const std::string& error);

            DDEConnectionManager* m_connectionManager;
            Logger& m_logger;
            app::services::OperationMonitorService* m_uiOpMonitor{nullptr};

            enum class State { Idle, FetchingSurfaceData, FetchingSagPoints, Completed, Failed };
            State m_state = State::Idle;
            std::string m_error;

            app::models::TaskSource m_source{app::models::TaskSource::None};
            uint64_t m_taskId{0};
            int m_targetSurface = 0;
            int m_targetSampling = 0;
            double m_targetAngle = 0.0;
            int m_sagPointIndex = 0;
            int m_surfaceRequestsRemaining = 0;
            int m_skippedPoints = 0;
            std::chrono::steady_clock::time_point m_calcStartTime;

            app::models::SurfaceData m_result;
            int m_totalDdeRequests{0};

            DWORD m_surfaceDataTimeoutMsOverride = 0;
            DWORD m_sagTimeoutMsOverride = 0;
    };

}
