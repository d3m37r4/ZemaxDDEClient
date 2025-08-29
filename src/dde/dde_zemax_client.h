#ifndef ZEMAX_DDE_CLIENT_H
#define ZEMAX_DDE_CLIENT_H

#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include "logger/logger.h"
#include "dde_zemax_types.h"

// DDE constants
#define DDE_APP_NAME            L"ZEMAX"        // Appname DDE
#define DDE_TOPIC               L"RayData"      // Topic DDE
#define DDE_TIMEOUT_MS          5000            // Timeout in ms (5 sec.)

namespace ZemaxDDE {
    class ZemaxDDEClient {
        public:
            using OnDDEConnectedCallback = std::function<void(ZemaxDDEClient*)>;

            ZemaxDDEClient(HWND zemaxDDEClient);
            ~ZemaxDDEClient();

            void initiateDDE();
            void terminateDDE();

            void setOnDDEConnectedCallback(OnDDEConnectedCallback callback);

            void getLensName();
            void getFileName();
            void getSystemData();
            void getFieldData();
            void getFieldByIndex(int fieldIndex);
            void getWaveData();
            void getWaveByIndex(int waveIndex);
            void getSurfaceRadius(int surfaceNumber);

            LRESULT handleDDEMessages(UINT iMsg, WPARAM wParam, LPARAM lParam);

            OpticalSystemData& getOpticalSystemData() { return opticalSystem; };

        private:
            HWND zemaxDDEServer = NULL;
            HWND zemaxDDEClient = NULL;
            OnDDEConnectedCallback onDDEConnected;
            OpticalSystemData opticalSystem;
            bool isDataReceived = false;

            void sendPostRequest(const char* request);
            void waitForData();
            void checkDDEConnection();
            void checkResponseStatus(const std::string& errorMsg);
    };

}
#endif // ZEMAX_DDE_CLIENT_H
