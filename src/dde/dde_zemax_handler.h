#ifndef DDE_ZEMAX_HANDLER_H
#define DDE_ZEMAX_HANDLER_H

#include <windows.h>
#include <string>
#include "logger/logger.h"

namespace ZemaxDDE {
    void initiateDDE(HWND hwnd);
    void terminateDDE();
    LRESULT handleDDEMessages(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

    double getSurfaceRadius(HWND hwnd, int surfaceNumber);
    void getLensName(HWND hwnd);
    void getFileName(HWND hwnd);
    void getSystemData(HWND hwnd);
    void getFieldData(HWND hwnd, int fieldIndex);
    void getWaveData(HWND hwnd, int waveIndex);

    extern HWND ZemaxDDEServer;
    extern bool IsDataReceived;
    extern std::string BufferString;
    extern char RequestItemBuffer[256];
    extern Logger logger;

    struct OpticalSystemData {
        char lensName[100] = {0};
        char fileName[100] = {0};
        int numSurfs = 0;
        int units = 0;
        int numFields = 0;
        int fieldType = 0;
        double xField[12] = {0};
        double yField[12] = {0};
        int numWaves = 0;
        int primWave = 0;
        double waveLen[24] = {0};
    };
    extern OpticalSystemData opticalSystem;

    constexpr wchar_t* DDE_APP_NAME = L"ZEMAX";
    constexpr wchar_t* DDE_TOPIC = L"RayData";
    constexpr DWORD DDE_TIMEOUT_MS = 10000;
}

#endif // DDE_ZEMAX_HANDLER_H
