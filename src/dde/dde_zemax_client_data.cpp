#include <format>
#include <dde.h>

#include "dde_zemax_client.h"
#include "dde_zemax_utils.h"

namespace ZemaxDDE {
    void ZemaxDDEClient::getLensName() {
        checkDDEConnection();

        m_isDataReceived = false;
        std::string request = std::format("GetName");

        sendPostRequest(request);
        checkResponseStatus(std::format("GetName: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getFileName() {
        checkDDEConnection();

        m_isDataReceived = false;
        std::string request = std::format("GetFile");

        sendPostRequest(request);
        checkResponseStatus(std::format("GetFile: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getSystemData() {
        checkDDEConnection();

        m_isDataReceived = false;
        std::string request = std::format("GetSystem");

        sendPostRequest(request);
        checkResponseStatus(std::format("GetSystem: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getFieldData() {
        checkDDEConnection();

        m_isDataReceived = false;
        std::string request = std::format("GetField,0");

        sendPostRequest(request);
        checkResponseStatus(std::format("GetField: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getFieldByIndex(int fieldIndex) {
        checkDDEConnection();

        m_isDataReceived = false;
        std::string request = std::format("GetField,{}", fieldIndex);

        sendPostRequest(request);
        checkResponseStatus(std::format("GetField: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getWaveData() {
        checkDDEConnection();

        m_isDataReceived = false;
        std::string request = std::format("GetWave,0");

        sendPostRequest(request);
        checkResponseStatus(std::format("GetWave: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getWaveByIndex(int waveIndex) {
        checkDDEConnection();

        m_isDataReceived = false;
        std::string request = std::format("GetWave,{}", waveIndex);

        sendPostRequest(request);
        checkResponseStatus(std::format("GetWave: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getSurfaceData(int surfaceNumber, int code, int arg2) {
        checkDDEConnection();

        m_isDataReceived = false;
        std::string request = std::format("GetSurfaceData,{},{},{}", surfaceNumber, code, arg2);

        sendPostRequest(request);
        checkResponseStatus(std::format("GetSurfaceData: No response from Zemax for request = {}", request));
    }

    void ZemaxDDEClient::getSag(int surfaceNumber, double x, double y) {
        checkDDEConnection();

        m_isDataReceived = false;
        std::string request = std::format("GetSag,{},{},{}", surfaceNumber, x, y);

        sendPostRequest(request);
        checkResponseStatus(std::format("GetSag: No response from Zemax for request = {}", request));
    }
}
