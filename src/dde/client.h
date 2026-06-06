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
            /// Threshold for emitting a mass-error diagnostic warning when
            /// multiple consecutive DDE requests fail with 'Zemax is not connected'
            /// in a single dispatchNext() invocation. 50 is large enough to
            /// avoid log spam for normal single failures but small enough to
            /// flag server-side outages in production.
            static constexpr size_t kMassErrorWarnThreshold = 50;

            ZemaxDDEClient(HWND hwndClient, Logger& logger);
            ~ZemaxDDEClient();

            void initiateDDE();
            void initiateDDE(HWND targetHwnd);
            void terminateDDE();

            bool isConnected() const noexcept { return m_hwndZemaxServer != nullptr; }

            void pumpMessages();
            void processTimeouts();

            /// Default timeout/retries used by submitRequest when caller passes
            /// the sentinel value (0 / -1). Updated by DDEConnectionManager from
            /// AppSettings.dde at runtime.
            void setDefaultTimeoutMs(DWORD ms) noexcept { m_defaultTimeoutMs = ms; }
            void setDefaultRetries(int n) noexcept { m_defaultRetries = n; }
            [[nodiscard]] DWORD getDefaultTimeoutMs() const noexcept { return m_defaultTimeoutMs; }
            [[nodiscard]] int   getDefaultRetries()   const noexcept { return m_defaultRetries; }

            LRESULT handleDDEMessages(UINT iMsg, WPARAM wParam, LPARAM lParam);

            using OnDDEConnectedCallback = std::function<void(ZemaxDDEClient*)>;
            void setOnDDEConnectedCallback(OnDDEConnectedCallback callback);

            /// Submits a DDE request for processing and starts if the pipeline is idle.
            /// @param timeoutMs 0 = use client's default (AppSettings.dde.connectionTimeoutMs).
            /// @param retries   -1 = use client's default (AppSettings.dde.maxRetryCount).
            /// @return Unique request ID for logging.
            uint64_t submitRequest(const std::string& command,
                std::function<void(const std::string&)> onSuccess,
                std::function<void(const std::string&)> onError,
                DWORD timeoutMs = 0,
                int retries = -1,
                const std::string& serviceId = "");

            // Getters
            [[nodiscard]] const OpticalSystemData& getOpticalSystemData() const noexcept { return m_opticalSystem; }
            [[nodiscard]] SurfaceData* getTolerancedSurface() noexcept { return &m_tolerancedSurface; }
            [[nodiscard]] SurfaceData* getNominalSurface() noexcept { return &m_nominalSurface; }
            [[nodiscard]] OperationMonitor* getOperationMonitor() noexcept { return m_operationMonitor.get(); }

        private:
            /// Takes the next request from the queue and sends it to Zemax.
            void dispatchNext();
            /// Converts command to UTF-16, creates a Win32 atom,
            /// and posts WM_DDE_REQUEST to the Zemax server window.
            void sendRequest(DdeRequest& req);
            /// Resets active request and advances to the next in queue.
            void finishRequest();
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

            DWORD m_defaultTimeoutMs = 1000;
            int   m_defaultRetries   = 1;
    };
}
