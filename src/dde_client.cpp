#include "dde_client.h"
#include <dde.h>
#include <stdexcept>
#include <cstdio>
#include <cstring>

namespace ZemaxDDE {
    std::vector<std::string>* debug_log = nullptr;

    void setDebugLog(std::vector<std::string>& log) {
        debug_log = &log;
    }

    void AddDebugLog(const char* msg) {
        if (debug_log) {
            debug_log->push_back(std::string(msg));
        }
    }

    static HWND hwndServer = NULL;
    static bool GotData = false;
    static char szBuffer[5000];
    static char szItem[256], szTest[256];

    char* GetString(char* szBuffer, int n, char* szSubString) {
        int i = 0, j = 0, k = 0;
        char szTest[1000] = {0};
        szSubString[0] = '\0';
        while (szBuffer[i] && k <= n) {
            szTest[j] = szBuffer[i];
            if (szBuffer[i] == '"') {
                i++;
                j++;
                szTest[j] = szBuffer[i];
                while (szBuffer[i] != '"' && szBuffer[i]) {
                    i++;
                    j++;
                    szTest[j] = szBuffer[i];
                }
            }
            if (szTest[j] == '\n' || szTest[j] == '\r' || szTest[j] == '\0' || szTest[j] == ',' || szTest[j] == ' ') {
                szTest[j] = '\0';
                if (k == n) {
                    strcpy(szSubString, szTest);
                    return szSubString;
                }
                k++;
                j = -1;
            }
            i++;
            j++;
        }
        szTest[j] = szBuffer[i];
        szTest[j] = '\0';
        if (k == n) strcpy(szSubString, szTest);
        return szSubString;
    }

    void WaitForData(HWND hwnd) {
        MSG msg;
        DWORD dwTime = GetTickCount();
        GotData = false;
        while ((GetTickCount() - dwTime < 10000) && !GotData) {
            while (PeekMessage(&msg, hwnd, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
                DispatchMessage(&msg);
            }
        }
    }

    void PostRequestMessage(const char* szItem, HWND hwndServer, HWND hwnd) {
        wchar_t wItem[256];
        MultiByteToWideChar(CP_ACP, 0, szItem, -1, wItem, 256);
        ATOM aItem = GlobalAddAtomW(wItem);
        if (!PostMessageW(hwndServer, WM_DDE_REQUEST, (WPARAM)hwnd, PackDDElParam(WM_DDE_REQUEST, CF_TEXT, aItem))) {
            GlobalDeleteAtom(aItem);
            throw std::runtime_error("Cannot communicate with Zemax");
        }
        WaitForData(hwnd);
    }

    void initiateDDE(HWND hwnd) {
        hwndServer = NULL;
        ATOM aApp = GlobalAddAtomW(L"ZEMAX");
        ATOM aTop = GlobalAddAtomW(L"RayData");
        SendMessageW(HWND_BROADCAST, WM_DDE_INITIATE, (WPARAM)hwnd, MAKELONG(aApp, aTop));
        if (debug_log) AddDebugLog("Sent WM_DDE_INITIATE with app=ZEMAX, topic=RayData");
        GlobalDeleteAtom(aApp);
        GlobalDeleteAtom(aTop);
        if (!hwndServer) {
            if (debug_log) AddDebugLog("No hwndServer received");
            throw std::runtime_error("Failed to establish DDE connection with Zemax");
        }
    }

    double getSurfaceRadius(HWND hwnd, int surfaceNumber) {
        if (!hwndServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        std::memset(szBuffer, 0, sizeof(szBuffer));
        GotData = false;

        char request[256];
        snprintf(request, sizeof(request), "GetSurfaceData,%d,2", surfaceNumber);
        if (debug_log) AddDebugLog((std::string("Sending request: ") + request).c_str());
        PostRequestMessage(request, hwndServer, hwnd);

        if (!GotData) {
            if (debug_log) AddDebugLog("No response from Zemax");
            throw std::runtime_error("No response from Zemax");
        }

        if (debug_log) AddDebugLog((std::string("Raw response: ") + szBuffer).c_str());
        char szSubString[256];
        GetString(szBuffer, 0, szSubString);
        if (debug_log) AddDebugLog((std::string("Parsed value: ") + szSubString).c_str());
        double curvature = atof(szSubString);
        double radius = (curvature != 0.0) ? 1.0 / curvature : 0.0;
        if (debug_log) AddDebugLog((std::string("Calculated radius: ") + std::to_string(radius)).c_str());
        return radius;
    }

    LRESULT handleDDEMessages(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
        static DDEACK DdeAck;
        GLOBALHANDLE hDdeData;
        DDEDATA* pDdeData;
        ATOM aItem;
        UINT_PTR uiLow, uiHi;

        if (debug_log) AddDebugLog((std::string("Received DDE message: ") + std::to_string(iMsg)).c_str());
        
        switch (iMsg) {
            case WM_DDE_ACK:
                if (!hwndServer) {
                    UnpackDDElParam(WM_DDE_ACK, lParam, &uiLow, &uiHi);
                    FreeDDElParam(WM_DDE_ACK, lParam);
                    hwndServer = (HWND)wParam;
                    if (debug_log) AddDebugLog((std::string("DDE_ACK received, hwndServer set to: ") + std::to_string((uintptr_t)hwndServer)).c_str());
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
                    WideCharToMultiByte(CP_ACP, 0, wItem, -1, szItem, sizeof(szItem), NULL, NULL);
                    GetString(szItem, 0, szTest);
                    GotData = true;
                    strncpy(szBuffer, (char*)pDdeData->Value, sizeof(szBuffer) - 1);
                    szBuffer[sizeof(szBuffer) - 1] = '\0';
                    if (debug_log) AddDebugLog((std::string("DDE_DATA received, content: ") + szBuffer).c_str());

                    if (strncmp(szTest, "GetSurfaceData", 14) == 0) {
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
                return 0;
        }
        return 0;
    }

    void terminateDDE() {
        if (hwndServer) {
            PostMessageW(hwndServer, WM_DDE_TERMINATE, 0, 0);
            hwndServer = NULL;
        }
    }
}
