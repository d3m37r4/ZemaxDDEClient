#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <windows.h>

#include "types.h"

class Logger;

namespace ZemaxDDE {
    class InitialDataLoadService;
    class OperationMonitor;

    struct DdeRequest {
        uint64_t id;
        std::string command;
        std::function<void(const std::string&)> onSuccess;
        std::function<void(const std::string&)> onError;
        DWORD timeoutMs;
        int retriesLeft;
        std::string serviceId;
        DWORD startTime = 0;
    };

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

            uint64_t enqueueRequest(const std::string& command,
                std::function<void(const std::string&)> onSuccess,
                std::function<void(const std::string&)> onError,
                DWORD timeoutMs = 1000,
                int retries = 1,
                const std::string& serviceId = "");

            // Getters
            [[nodiscard]] const OpticalSystemData& getOpticalSystemData() const noexcept { return m_opticalSystem; }
            [[nodiscard]] SurfaceData* getTolerancedSurface() noexcept { return &m_tolerancedSurface; }
            [[nodiscard]] SurfaceData* getNominalSurface() noexcept { return &m_nominalSurface; }
            [[nodiscard]] OperationMonitor* getOperationMonitor() noexcept { return m_operationMonitor.get(); }

        private:
            void dequeueAndSend();
            void checkDDEConnection();

            HWND m_hwndZemaxServer = nullptr;
            HWND m_hwndZemaxClient = nullptr;
            Logger& m_logger;
            OnDDEConnectedCallback m_onDDEConnected;
            OpticalSystemData m_opticalSystem;
            SurfaceData m_tolerancedSurface;
            SurfaceData m_nominalSurface;

            std::unique_ptr<InitialDataLoadService> m_initialDataLoad;
            std::unique_ptr<OperationMonitor> m_operationMonitor;

            std::deque<DdeRequest> m_requestQueue;
            std::optional<DdeRequest> m_activeRequest;
            uint64_t m_nextRequestId = 1;
    };
}
