#pragma once

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
            void loadFields(int numFields);
            void loadWaves(int numWaves);
            void onError(const std::string& msg);
            void checkCompletion();

            ZemaxDDEClient& m_client;
            OpticalSystemData& m_opticalSystem;
            Logger& m_logger;

            LoadState m_state = LoadState::Idle;
            std::string m_error;
            int m_pendingRequests = 0;
    };

}
