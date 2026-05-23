#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

#include "types.h"

class Logger;

namespace ZemaxDDE {
    class InitialDataLoadService;

    class ZemaxDDEClient {
        public:
            ZemaxDDEClient(HWND hwndClient, Logger& logger);
            ~ZemaxDDEClient();

            void initiateDDE();
            void initiateDDE(HWND targetHwnd);
            void terminateDDE();

            bool isConnected() const noexcept { return m_hwndZemaxServer != nullptr; }

            void pumpMessages();
            void processTimeouts();

            LRESULT handleDDEMessages(UINT iMsg, WPARAM wParam, LPARAM lParam);

            using OnDDEConnectedCallback = std::function<void(ZemaxDDEClient*)>;
            void setOnDDEConnectedCallback(OnDDEConnectedCallback callback);

            struct PendingRequest {
                uint64_t id;
                std::string command;
                std::string rawRequest;
                std::function<void(const std::string&)> onResult;
                std::function<void(const std::string&)> onError;
                int retryCount = 0;
                DWORD startTime = 0;
            };

            uint64_t sendRequest(const std::string& command,
                std::function<void(const std::string&)> onResult,
                std::function<void(const std::string&)> onError);

            // Getters
            [[nodiscard]] const OpticalSystemData& getOpticalSystemData() const noexcept { return m_opticalSystem; }
            [[nodiscard]] SurfaceData* getTolerancedSurface() noexcept { return &m_tolerancedSurface; }
            [[nodiscard]] SurfaceData* getNominalSurface() noexcept { return &m_nominalSurface; }

        private:
            HWND m_hwndZemaxServer = nullptr;
            HWND m_hwndZemaxClient = nullptr;
            Logger& m_logger;
            OnDDEConnectedCallback m_onDDEConnected;
            OpticalSystemData m_opticalSystem;
            SurfaceData m_tolerancedSurface;
            SurfaceData m_nominalSurface;

            std::unique_ptr<InitialDataLoadService> m_initialDataLoad;

            std::vector<PendingRequest> m_pendingRequests;
            uint64_t m_nextRequestId = 1;

            void checkDDEConnection();
    };
}
