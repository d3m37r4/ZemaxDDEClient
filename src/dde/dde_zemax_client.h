#ifndef ZEMAX_DDE_CLIENT_H
#define ZEMAX_DDE_CLIENT_H

#include <windows.h>
#include <string>
#include <vector>
#include "logger/logger.h"
#include "dde_zemax_types.h"

// DDE constants
#define DDE_APP_NAME L"ZEMAX"   // Appname DDE
#define DDE_TOPIC L"RayData"    // Topic DDE
#define DDE_TIMEOUT_MS 5000     // Timeout in ms (5 sec.)

namespace ZemaxDDE {
    class ZemaxDDEClient {
    public:
        ZemaxDDEClient(HWND zemaxDDEClient);
        ~ZemaxDDEClient();

        void initiateDDE();
        void terminateDDE();

        void getLensName();
        void getFileName();
        void getSystemData();
        void getFieldData(int fieldIndex);
        void getWaveData(int waveIndex);
        void getSurfaceRadius(int surfaceNumber);

        LRESULT handleDDEMessages(UINT iMsg, WPARAM wParam, LPARAM lParam);

        OpticalSystemData& getOpticalSystemData() { return opticalSystem; }

    private:
        Logger logger;
        OpticalSystemData opticalSystem;

        HWND zemaxDDEServer = NULL;
        HWND zemaxDDEClient = NULL;
        bool isDataReceived = false;

        void sendPostRequest(const char* request);
        void waitForData();
        void checkDDEConnection();
        void checkResponseStatus(const std::string& errorMsg);
    };

}
#endif // ZEMAX_DDE_CLIENT_H
