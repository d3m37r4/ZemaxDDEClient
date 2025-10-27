#pragma once

#include <windows.h>
#include <string>
#include <vector>
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
            void clearTolerancedSurface() { tolerancedSurface = SurfaceData{}; }
            void clearNominalSurface() { nominalSurface = SurfaceData{}; }

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
