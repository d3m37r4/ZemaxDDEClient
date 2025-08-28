#include <dde.h>
#include "dde_zemax_client.h"
#include "dde_zemax_utils.h"

namespace ZemaxDDE {
    void ZemaxDDEClient::getLensName() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetName");

        sendPostRequest(request);
        checkResponseStatus(std::string("GetName: No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getFileName() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetFile");

        sendPostRequest(request);
        checkResponseStatus(std::string("GetFile: No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getSystemData() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetSystem");

        sendPostRequest(request);
        checkResponseStatus(std::string("GetSystem: No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getFieldData() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetField,0");

        sendPostRequest(request);
        checkResponseStatus(std::string("GetField: No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getFieldByIndex(int fieldIndex) {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetField,%d", fieldIndex);

        sendPostRequest(request);
        checkResponseStatus(std::string("GetField: No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getWaveData() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetWave,0");

        sendPostRequest(request);
        checkResponseStatus(std::string("GetWave: No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getWaveByIndex(int waveIndex) {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetWave,%d", waveIndex);

        sendPostRequest(request);
        checkResponseStatus(std::string("GetWave: No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getSurfaceRadius(int surfaceNumber) {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetSurfaceData,%d,2", surfaceNumber);

        sendPostRequest(request);
        checkResponseStatus(std::string("GetSurfaceData: No response from Zemax for request = ") + request);
    }
}
