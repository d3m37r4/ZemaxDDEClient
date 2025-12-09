#include <fstream>
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
        checkResponseStatus(std::format("GetName: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getFileName() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetFile");

        sendPostRequest(request);
        checkResponseStatus(std::format("GetFile: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getSystemData() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetSystem");

        sendPostRequest(request);
        checkResponseStatus(std::format("GetSystem: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getFieldData() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetField,0");

        sendPostRequest(request);
        checkResponseStatus(std::format("GetField: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getFieldByIndex(int fieldIndex) {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetField,%d", fieldIndex);

        sendPostRequest(request);
        checkResponseStatus(std::format("GetField: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getWaveData() {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetWave,0");

        sendPostRequest(request);
        checkResponseStatus(std::format("GetWave: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getWaveByIndex(int waveIndex) {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetWave,%d", waveIndex);

        sendPostRequest(request);
        checkResponseStatus(std::format("GetWave: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getSurfaceData(int surfaceNumber, int code, int arg2) {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetSurfaceData,%d,%d,%d", surfaceNumber, code, arg2);

        sendPostRequest(request);
        checkResponseStatus(std::format("GetSurfaceData: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getSag(int surfaceNumber, double x, double y) {
        checkDDEConnection();

        isDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetSag,%d,%f,%f", surfaceNumber, x, y);

        sendPostRequest(request);
        checkResponseStatus(std::format("GetSag: No response from Zemax for request = {}", request));
    }
}
