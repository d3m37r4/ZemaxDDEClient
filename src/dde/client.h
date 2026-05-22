#pragma once

#include <string_view>
#include <windows.h>
#include <functional>

#include "types.h"

class Logger;

namespace ZemaxDDE {
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

            // DDE Commands
            void getLensName();
            void getFileName();
            void getSystemData();
            void getFieldData();
            void getFieldByIndex(int fieldIndex);
            void getWaveData();
            void getWaveByIndex(int waveIndex);
            void getSurfaceData(int surfaceNumber, int code, int arg2 = 0);
            void getSag(int surfaceNumber, double x = 0.0, double y = 0.0);

            // Getters
            [[nodiscard]] const OpticalSystemData& getOpticalSystemData() const noexcept { return m_opticalSystem; }
            [[nodiscard]] SurfaceData* getTolerancedSurface() noexcept { return &m_tolerancedSurface; }
            [[nodiscard]] SurfaceData* getNominalSurface() noexcept { return &m_nominalSurface; }
            [[nodiscard]] SurfaceData* getCurrentStorage() noexcept { return m_currentStorage; }

            // Setters
            void setStorageTarget(SurfaceData* targetStorage) noexcept { m_currentStorage = targetStorage; }
            void setSurfaceProfileMetadata(const SurfaceProfileMetadata& metadata) {
                if (m_currentStorage) {
                    m_currentStorage->angle = metadata.angle;
                    m_currentStorage->sampling = metadata.sampling;
                }
            }

        private:
            HWND m_hwndZemaxServer = nullptr;
            HWND m_hwndZemaxClient = nullptr;
            Logger& m_logger;
            OnDDEConnectedCallback m_onDDEConnected;
            OpticalSystemData m_opticalSystem;
            SurfaceData m_tolerancedSurface;
            SurfaceData m_nominalSurface;
            SurfaceData* m_currentStorage = &m_tolerancedSurface;
            bool m_isDataReceived{false};

            void sendPostRequest(std::string_view request);
            void waitForData();
            void checkDDEConnection();
            void checkResponseStatus(const std::string& errorMsg);
    };
}
