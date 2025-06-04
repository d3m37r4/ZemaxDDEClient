#include <windows.h>
#include <dde.h>
#include <stdio.h>
#include "zemax_dde.h"

#define DDE_TIMEOUT 5000
static HWND hwndServer = NULL;
static int GotData = 0;
static char szGlobalBuffer[5000];

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
void WaitForData(HWND hwnd);
void PostRequestMessage(char* szItem, HWND hwndServer, HWND hwnd);
DWORD WINAPI DDEMessageThread(LPVOID lpParam);

int initialize_dde(HWND hwnd) {
    if (hwndServer) return 1;

    ATOM aApp = GlobalAddAtom("ZEMAX");
    ATOM aTop = GlobalAddAtom("RayData");
    SendMessage(HWND_BROADCAST, WM_DDE_INITIATE, (WPARAM)hwnd, MAKELONG(aApp, aTop));
    GlobalDeleteAtom(aApp);
    GlobalDeleteAtom(aTop);

    // Ждём ответа на инициализацию
    MSG msg;
    DWORD startTime = GetTickCount();
    while (GetTickCount() - startTime < DDE_TIMEOUT && !hwndServer) {
        if (PeekMessage(&msg, hwnd, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
            DispatchMessage(&msg);
        }
        Sleep(10);
    }

    if (!hwndServer) {
        return 0; // Соединение не установлено
    }
    return 1;
}

const char* send_zemax_request(const char* item) {
    static char result[256] = "";
    char request[256];
    snprintf(request, sizeof(request), "GetSurfaceData, %s, 2", item); // Запрос радиуса (индекс 2 для радиуса)

    if (!hwndServer) {
        strcpy(result, "DDE not initialized");
        return result;
    }

    szGlobalBuffer[0] = '\0';
    GotData = 0;
    PostRequestMessage(request, hwndServer, hwndServer);

    if (GotData) {
        strcpy(result, szGlobalBuffer);
    } else {
        strcpy(result, "Failed to get data");
    }

    return result;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    switch (iMsg) {
        case WM_DDE_ACK:
            if (!hwndServer) {
                UINT_PTR uiLow, uiHi;
                UnpackDDElParam(WM_DDE_ACK, lParam, &uiLow, &uiHi);
                FreeDDElParam(WM_DDE_ACK, lParam);
                hwndServer = (HWND)wParam;
                GlobalDeleteAtom((ATOM)uiLow);
                GlobalDeleteAtom((ATOM)uiHi);
            }
            return 0;

        case WM_DDE_DATA:
            if (lParam) {
                UINT_PTR uiLow, uiHi;
                UnpackDDElParam(WM_DDE_DATA, lParam, &uiLow, &uiHi);
                FreeDDElParam(WM_DDE_DATA, lParam);
                GLOBALHANDLE hDdeData = (GLOBALHANDLE)uiLow;
                DDEDATA* pDdeData = (DDEDATA*)GlobalLock(hDdeData);
                if (pDdeData && pDdeData->cfFormat == CF_TEXT) {
                    strncpy(szGlobalBuffer, (char*)pDdeData->Value, sizeof(szGlobalBuffer) - 1);
                    szGlobalBuffer[sizeof(szGlobalBuffer) - 1] = '\0';
                    GotData = 1;
                }
                GlobalUnlock(hDdeData);
                GlobalFree(hDdeData);
            }
            return 0;

        case WM_DESTROY:
            if (hwndServer) PostMessage(hwndServer, WM_DDE_TERMINATE, (WPARAM)hwnd, 0L);
            hwndServer = NULL;
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void WaitForData(HWND hwnd) {
    DWORD dwTime = GetTickCount();
    GotData = 0;
    while ((GetTickCount() - dwTime < DDE_TIMEOUT) && !GotData) {
        MSG msg;
        while (PeekMessage(&msg, hwnd, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
            DispatchMessage(&msg);
        }
        Sleep(10);
    }
}

void PostRequestMessage(char* szItem, HWND hwndServer, HWND hwnd) {
    ATOM aItem = GlobalAddAtom(szItem);
    if (!PostMessage(hwndServer, WM_DDE_REQUEST, (WPARAM)hwnd, PackDDElParam(WM_DDE_REQUEST, CF_TEXT, aItem))) {
        GlobalDeleteAtom(aItem);
        return;
    }
    WaitForData(hwnd);
    GlobalDeleteAtom(aItem);
}

DWORD WINAPI DDEMessageThread(LPVOID lpParam) {
    MSG msg;
    while (GetMessage(&msg, (HWND)lpParam, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
