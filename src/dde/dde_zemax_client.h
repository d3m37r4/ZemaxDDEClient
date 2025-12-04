#pragma once

#include <windows.h>
#include <functional>

#include "logger/logger.h"

#include "dde_zemax_types.h"

namespace ZemaxDDE {
    class ZemaxDDEClient {
        public:
            ZemaxDDEClient(HWND zemaxDDEClient);
            ~ZemaxDDEClient();

            void initiateDDE();
            void terminateDDE();

            bool isConnected() const { return zemaxDDEServer != NULL; }

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
            const StorageTarget& getStorageTarget() const { return currentStorageTarget; }
            const OpticalSystemData& getOpticalSystemData() const { return opticalSystem; }
            const SurfaceData& getTolerancedSurface() const { return tolerancedSurface; }
            const SurfaceData& getNominalSurface() const { return nominalSurface; }

            // Setters
            void setStorageTarget(StorageTarget target) { currentStorageTarget = target; }
            void clearTolerancedSurface() { tolerancedSurface.clear(); }
            void clearNominalSurface() { nominalSurface.clear(); }
            void setSurfaceProfileMetadata(ZemaxDDE::StorageTarget target, const SurfaceProfileMetadata& metadata) {
                SurfaceData& surface = (target == StorageTarget::NOMINAL) ? nominalSurface : tolerancedSurface;
                surface.angle = metadata.angle;
                surface.sampling = metadata.sampling;
            }

        private:
            HWND zemaxDDEServer = NULL;
            HWND zemaxDDEClient = NULL;
            OnDDEConnectedCallback onDDEConnected;
            OpticalSystemData opticalSystem;
            SurfaceData tolerancedSurface;
            SurfaceData nominalSurface;
            StorageTarget currentStorageTarget = StorageTarget::TOLERANCED;
            bool isDataReceived = false;

            void sendPostRequest(const char* request);
            void waitForData();
            void checkDDEConnection();
            void checkResponseStatus(const std::string& errorMsg);
    };
}
