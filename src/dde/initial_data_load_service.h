#pragma once

#include <chrono>
#include <string>

class Logger;

namespace ZemaxDDE {

    class ZemaxDDEClient;
    struct OpticalSystemData;

    enum class LoadState {
        Idle,
        LoadingSystem,
        LoadingFields,
        LoadingWaves,
        Completed,
        Failed
    };

    class InitialDataLoadService {
        public:
            InitialDataLoadService(ZemaxDDEClient& client, OpticalSystemData& opticalSystem, Logger& logger);

            void start();
            LoadState getState() const { return m_state; }
            const std::string& getError() const { return m_error; }

        private:
            void loadSystem();
            void loadNextField();
            void loadNextWave();
            void onError(const std::string& msg);

            ZemaxDDEClient& m_client;
            OpticalSystemData& m_opticalSystem;
            Logger& m_logger;

            LoadState m_state = LoadState::Idle;
            std::string m_error;
            int m_currentField = 0;
            int m_totalFields = 0;
            int m_currentWave = 0;
            int m_totalWaves = 0;
            std::chrono::steady_clock::time_point m_startTime;
    };

}
