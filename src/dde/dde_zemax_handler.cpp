#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <dde.h>
#include "dde_zemax_handler.h"

namespace ZemaxDDE {
    Logger logger;
    OpticalSystemData opticalSystem;

    HWND hwndServer = NULL;
    bool GotData = false;
    char buffer[5000] = {0};
    char item[256] = {0};
    char test[256] = {0};

    char* GetString(char* buffer, int n, char* subString) {
        int i = 0, j = 0, k = 0;
        char temp[1000] = {0};
        subString[0] = '\0';
        while (buffer[i] && k <= n) {
            temp[j] = buffer[i];
            if (buffer[i] == '"') {
                i++;
                j++;
                temp[j] = buffer[i];
                while (buffer[i] != '"' && buffer[i]) {
                    i++;
                    j++;
                    temp[j] = buffer[i];
                }
            }
            if (temp[j] == '\n' || temp[j] == '\r' || temp[j] == '\0' || temp[j] == ',' || temp[j] == ' ') {
                temp[j] = '\0';
                if (k == n) {
                    strcpy(subString, temp);
                    return subString;
                }
                k++;
                j = -1;
            }
            i++;
            j++;
        }
        temp[j] = buffer[i];
        temp[j] = '\0';
        if (k == n) strcpy(subString, temp);
        return subString;
    }

    void WaitForData(HWND hwnd) {
        MSG msg;
        DWORD dwTime = GetTickCount();
        GotData = false;
        while ((GetTickCount() - dwTime < DDE_TIMEOUT_MS) && !GotData) {
            while (PeekMessage(&msg, hwnd, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
                DispatchMessage(&msg);
            }
        }
    }

    void PostRequestMessage(const char* item, HWND hwndServer, HWND hwnd) {
        wchar_t wItem[256];
        MultiByteToWideChar(CP_ACP, 0, item, -1, wItem, 256);
        ATOM aItem = GlobalAddAtomW(wItem);
        if (!PostMessageW(hwndServer, WM_DDE_REQUEST, (WPARAM)hwnd, PackDDElParam(WM_DDE_REQUEST, CF_TEXT, aItem))) {
            GlobalDeleteAtom(aItem);
            throw std::runtime_error("Cannot communicate with Zemax");
        }
        WaitForData(hwnd);
    }

    void initiateDDE(HWND hwnd) {
        hwndServer = NULL;
        ATOM aApp = GlobalAddAtomW(DDE_APP_NAME);
        ATOM aTop = GlobalAddAtomW(DDE_TOPIC);
        SendMessageW(HWND_BROADCAST, WM_DDE_INITIATE, (WPARAM)hwnd, MAKELONG(aApp, aTop));
        logger.addLog("Sent WM_DDE_INITIATE with app=ZEMAX, topic=RayData");
        GlobalDeleteAtom(aApp);
        GlobalDeleteAtom(aTop);
        if (!hwndServer) {
            logger.addLog("No hwndServer received");
            throw std::runtime_error("Failed to establish DDE connection with Zemax");
        }
    }

    double getSurfaceRadius(HWND hwnd, int surfaceNumber) {
        if (!hwndServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        GotData = false;
        char request[256];
        snprintf(request, sizeof(request), "GetSurfaceData,%d,2", surfaceNumber);
        logger.addLog(std::string("Sending request: ") + request);
        PostRequestMessage(request, hwndServer, hwnd);

        if (!GotData) {
            logger.addLog("No response from Zemax");
            throw std::runtime_error("No response from Zemax");
        }

        logger.addLog(std::string("Raw response: ") + buffer);
        char subString[256];
        GetString(buffer, 0, subString);
        logger.addLog(std::string("Parsed value: ") + subString);

        double curvature = atof(subString);
        double radius = (curvature != 0.0) ? 1.0 / curvature : 0.0;
        logger.addLog("Calculated radius: " + std::to_string(radius));

        return radius;
    }

    void getLensName(HWND hwnd) {
        if (!hwndServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        GotData = false;
        logger.addLog("Sending request: GetName");
        PostRequestMessage("GetName", hwndServer, hwnd);

        if (!GotData) {
            logger.addLog("No response from Zemax for GetName");
            throw std::runtime_error("No response from Zemax for GetName");
        }

        logger.addLog(std::string("Raw response for GetName: ") + buffer);
        strncpy(opticalSystem.lensName, buffer, sizeof(opticalSystem.lensName) - 1);
        opticalSystem.lensName[sizeof(opticalSystem.lensName) - 1] = '\0';
        if (strlen(opticalSystem.lensName) >= 2) opticalSystem.lensName[strlen(opticalSystem.lensName) - 2] = '\0';
        logger.addLog(std::string("Parsed Lens Name: ") + opticalSystem.lensName);
    }

    void getFileName(HWND hwnd) {
        if (!hwndServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        GotData = false;
        logger.addLog("Sending request: GetFile");
        PostRequestMessage("GetFile", hwndServer, hwnd);

        if (!GotData) {
            logger.addLog("No response from Zemax for GetFile");
            throw std::runtime_error("No response from Zemax for GetFile");
        }

        logger.addLog(std::string("Raw response for GetFile: ") + buffer);
        strncpy(opticalSystem.fileName, buffer, sizeof(opticalSystem.fileName) - 1);
        opticalSystem.fileName[sizeof(opticalSystem.fileName) - 1] = '\0';
        if (strlen(opticalSystem.fileName) >= 2) opticalSystem.fileName[strlen(opticalSystem.fileName) - 2] = '\0';
        logger.addLog(std::string("Parsed File Name: ") + opticalSystem.fileName);
    }

    void getSystemData(HWND hwnd) {
        if (!hwndServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        GotData = false;
        logger.addLog("Sending request: GetSystem");
        PostRequestMessage("GetSystem", hwndServer, hwnd);

        if (!GotData) {
            logger.addLog("No response from Zemax for GetSystem");
            throw std::runtime_error("No response from Zemax for GetSystem");
        }

        logger.addLog(std::string("Raw response for GetSystem: ") + buffer);
        char subString[256];
        opticalSystem.numSurfs = atoi(GetString(buffer, 0, subString));
        opticalSystem.units = atoi(GetString(buffer, 1, subString));
        logger.addLog("Parsed numSurfs: " + std::to_string(opticalSystem.numSurfs));
        logger.addLog("Parsed units: " + std::to_string(opticalSystem.units));
    }

    void getFieldData(HWND hwnd, int fieldIndex) {
        if (!hwndServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        GotData = false;
        char request[256];
        snprintf(request, sizeof(request), "GetField,%d", fieldIndex);
        logger.addLog(std::string("Sending request: ") + request);
        PostRequestMessage(request, hwndServer, hwnd);

        if (!GotData) {
            logger.addLog("No response from Zemax for GetField");
            throw std::runtime_error("No response from Zemax for GetField");
        }

        logger.addLog(std::string("Raw response for GetField: ") + buffer);
        char subString[256];
        if (fieldIndex == 0) {
            opticalSystem.fieldType = atoi(GetString(buffer, 0, subString));
            opticalSystem.numFields = atoi(GetString(buffer, 1, subString));
            logger.addLog("Parsed fieldType: " + std::to_string(opticalSystem.fieldType));
            logger.addLog("Parsed numFields: " + std::to_string(opticalSystem.numFields));
        } else if (fieldIndex <= 15) {
            opticalSystem.xField[fieldIndex] = atof(GetString(buffer, 0, subString));
            opticalSystem.yField[fieldIndex] = atof(GetString(buffer, 1, subString));
            logger.addLog("Parsed xField[" + std::to_string(fieldIndex) + "]: " + std::to_string(opticalSystem.xField[fieldIndex]));
            logger.addLog("Parsed yField[" + std::to_string(fieldIndex) + "]: " + std::to_string(opticalSystem.yField[fieldIndex]));
        }
    }

    void getWaveData(HWND hwnd, int waveIndex) {
        if (!hwndServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        GotData = false;
        char request[256];
        snprintf(request, sizeof(request), "GetWave,%d", waveIndex);
        logger.addLog(std::string("Sending request: ") + request);
        PostRequestMessage(request, hwndServer, hwnd);

        if (!GotData) {
            logger.addLog("No response from Zemax for GetWave");
            throw std::runtime_error("No response from Zemax for GetWave");
        }

        logger.addLog(std::string("Raw response for GetWave: ") + buffer);
        char subString[256];
        if (waveIndex == 0) {
            opticalSystem.primWave = atoi(GetString(buffer, 0, subString));
            opticalSystem.numWaves = atoi(GetString(buffer, 1, subString));
            logger.addLog("Parsed primWave: " + std::to_string(opticalSystem.primWave));
            logger.addLog("Parsed numWaves: " + std::to_string(opticalSystem.numWaves));
        } else if (waveIndex <= 15) {
            opticalSystem.waveLen[waveIndex] = atof(GetString(buffer, 0, subString));
            logger.addLog("Parsed waveLen[" + std::to_string(waveIndex) + "]: " + std::to_string(opticalSystem.waveLen[waveIndex]));
        }
    }

    LRESULT handleDDEMessages(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
        static DDEACK DdeAck;
        GLOBALHANDLE hDdeData;
        DDEDATA* pDdeData;
        ATOM aItem;
        UINT_PTR uiLow, uiHi;

        logger.addLog("Received DDE message: " + std::to_string(iMsg));
        
        switch (iMsg) {
            case WM_DDE_ACK:
                if (!hwndServer) {
                    UnpackDDElParam(WM_DDE_ACK, lParam, &uiLow, &uiHi);
                    FreeDDElParam(WM_DDE_ACK, lParam);
                    hwndServer = (HWND)wParam;
                    logger.addLog("DDE_ACK received, hwndServer set to: " + std::to_string((uintptr_t)hwndServer));
                    GlobalDeleteAtom((ATOM)uiLow);
                    GlobalDeleteAtom((ATOM)uiHi);
                }
                return 0;

            case WM_DDE_DATA:
                UnpackDDElParam(WM_DDE_DATA, lParam, &uiLow, &uiHi);
                FreeDDElParam(WM_DDE_DATA, lParam);
                hDdeData = (GLOBALHANDLE)(uintptr_t)uiLow;
                pDdeData = (DDEDATA*)GlobalLock(hDdeData);
                aItem = (ATOM)uiHi;

                DdeAck.bAppReturnCode = 0;
                DdeAck.reserved = 0;
                DdeAck.fBusy = FALSE;
                DdeAck.fAck = FALSE;

                if (pDdeData->cfFormat == CF_TEXT) {
                    wchar_t wItem[256];
                    GlobalGetAtomNameW(aItem, wItem, sizeof(wItem) / sizeof(wchar_t));
                    WideCharToMultiByte(CP_ACP, 0, wItem, -1, item, sizeof(item), NULL, NULL);
                    GetString(item, 0, test);
                    GotData = true;
                    strncpy(buffer, (char*)pDdeData->Value, sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';
                    logger.addLog(std::string("DDE_DATA received, content: ") + buffer);

                    if (strncmp(test, "GetSurfaceData", 14) == 0) {
                        DdeAck.fAck = TRUE;
                    }
                    if (strcmp(test, "GetName") == 0) {
                        strncpy(opticalSystem.lensName, (char*)pDdeData->Value, sizeof(opticalSystem.lensName) - 1);
                        opticalSystem.lensName[sizeof(opticalSystem.lensName) - 1] = '\0';
                        if (strlen(opticalSystem.lensName) >= 2) opticalSystem.lensName[strlen(opticalSystem.lensName) - 2] = '\0';
                        DdeAck.fAck = TRUE;
                    }
                    if (strcmp(test, "GetFile") == 0) {
                        strncpy(opticalSystem.fileName, (char*)pDdeData->Value, sizeof(opticalSystem.fileName) - 1);
                        opticalSystem.fileName[sizeof(opticalSystem.fileName) - 1] = '\0';
                        if (strlen(opticalSystem.fileName) >= 2) opticalSystem.fileName[strlen(opticalSystem.fileName) - 2] = '\0';
                        DdeAck.fAck = TRUE;
                    }
                    if (strcmp(test, "GetSystem") == 0) {
                        opticalSystem.numSurfs = atoi(GetString(buffer, 0, test));
                        opticalSystem.units = atoi(GetString(buffer, 1, test));
                        DdeAck.fAck = TRUE;
                    }
                    if (strcmp(test, "GetField") == 0) {
                        int fn;
                        GetString(item, 1, test);
                        fn = atoi(test);
                        if (fn == 0) {
                            opticalSystem.fieldType = atoi(GetString(buffer, 0, test));
                            opticalSystem.numFields = atoi(GetString(buffer, 1, test));
                        } else if (fn <= 15) {
                            opticalSystem.xField[fn] = atof(GetString(buffer, 0, test));
                            opticalSystem.yField[fn] = atof(GetString(buffer, 1, test));
                        }
                        DdeAck.fAck = TRUE;
                    }
                    if (strcmp(test, "GetWave") == 0) {
                        int wn;
                        GetString(item, 1, test);
                        wn = atoi(test);
                        if (wn == 0) {
                            opticalSystem.primWave = atoi(GetString(buffer, 0, test));
                            opticalSystem.numWaves = atoi(GetString(buffer, 1, test));
                        } else if (wn <= 15) {
                            opticalSystem.waveLen[wn] = atof(GetString(buffer, 0, test));
                        }
                        DdeAck.fAck = TRUE;
                    }
                }

                if (pDdeData->fAckReq == TRUE) {
                    WORD wStatus = *((WORD*)&DdeAck);
                    if (!PostMessageW((HWND)wParam, WM_DDE_ACK, (WPARAM)hwnd, PackDDElParam(WM_DDE_ACK, wStatus, aItem))) {
                        GlobalDeleteAtom(aItem);
                        GlobalUnlock(hDdeData);
                        GlobalFree(hDdeData);
                        return 0;
                    }
                } else {
                    GlobalDeleteAtom(aItem);
                }

                if (pDdeData->fRelease == TRUE || DdeAck.fAck == FALSE) {
                    GlobalUnlock(hDdeData);
                    GlobalFree(hDdeData);
                } else {
                    GlobalUnlock(hDdeData);
                }
                return 0;

            case WM_DDE_TERMINATE:
                PostMessageW(hwndServer, WM_DDE_TERMINATE, (WPARAM)hwnd, 0L);
                hwndServer = NULL;
                logger.addLog("DDE_TERMINATE received");
                return 0;
        }
        return 0;
    }

    void terminateDDE() {
        if (hwndServer) {
            PostMessageW(hwndServer, WM_DDE_TERMINATE, 0, 0);
            hwndServer = NULL;
            logger.addLog("DDE connection terminated");
        }
    }
}
