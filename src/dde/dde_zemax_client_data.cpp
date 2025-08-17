#include <dde.h>
#include "dde_zemax_client.h"
#include "dde_zemax_utils.h"

namespace ZemaxDDE {
    void ZemaxDDEClient::getSurfaceRadius(int surfaceNumber) {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetSurfaceData,%d,2", surfaceNumber);

        sendPostRequest(request);
        checkResponseStatus(std::string("No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getLensName() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetName");

        sendPostRequest(request);
        checkResponseStatus(std::string("No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getFileName() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetFile");

        sendPostRequest(request);
        checkResponseStatus(std::string("No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getSystemData() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetSystem");

        sendPostRequest(request);
        checkResponseStatus(std::string("No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getFieldData(int fieldIndex) {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetField,%d", fieldIndex);

        sendPostRequest(request);
        checkResponseStatus(std::string("No response from Zemax for request = ") + request);
    }

    void ZemaxDDEClient::getWaveData(int waveIndex) {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetWave,%d", waveIndex);

        sendPostRequest(request);
        checkResponseStatus(std::string("No response from Zemax for request = ") + request);
    }

}
