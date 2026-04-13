#pragma once

#include <string_view>
#include <windows.h>
#include <functional>

#include "logger/logger.h"

#include "types.h"

namespace ZemaxDDE {
    class ZemaxDDEClient {
        public:
            ZemaxDDEClient(HWND zemaxDDEClient);
            ~ZemaxDDEClient();

            void initiateDDE();
            void terminateDDE();

            bool isConnected() const noexcept { return m_hwndZemaxServer != NULL; }

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
            [[nodiscard]] const StorageTarget& getStorageTarget() const noexcept { return m_currentStorageTarget; }
            [[nodiscard]] const OpticalSystemData& getOpticalSystemData() const noexcept { return m_opticalSystem; }
            [[nodiscard]] const SurfaceData& getTolerancedSurface() const noexcept { return m_tolerancedSurface; }
            [[nodiscard]] const SurfaceData& getNominalSurface() const noexcept { return m_nominalSurface; }

            // Setters
            void setStorageTarget(StorageTarget target) noexcept { m_currentStorageTarget = target; }
            void clearTolerancedSurface() noexcept { m_tolerancedSurface.clear(); }
            void clearNominalSurface() noexcept { m_nominalSurface.clear(); }
            void setSurfaceProfileMetadata(ZemaxDDE::StorageTarget target, const SurfaceProfileMetadata& metadata) {
                SurfaceData& surface = (target == StorageTarget::NOMINAL) ? m_nominalSurface : m_tolerancedSurface;
                surface.angle = metadata.angle;
                surface.sampling = metadata.sampling;
            }

        private:
            HWND m_hwndZemaxServer = NULL;
            HWND m_hwndZemaxClient = NULL;
            OnDDEConnectedCallback m_onDDEConnected;
            OpticalSystemData m_opticalSystem;
            SurfaceData m_tolerancedSurface;
            SurfaceData m_nominalSurface;
            StorageTarget m_currentStorageTarget = StorageTarget::TOLERANCED;
            bool m_isDataReceived = false;

            void sendPostRequest(std::string_view request);
            void waitForData();
            void checkDDEConnection();
            void checkResponseStatus(const std::string& errorMsg);
    };
}
