#include <windows.h>
#include <dde.h>
#include <stdio.h>
#include <string.h>
#include "zemax_dde.h"

#define DDE_TIMEOUT 5000
static HWND hwndServer = NULL;
static int GotData = 0;
static char szGlobalBuffer[5000];

// Глобальные переменные для числа поверхностей и единиц измерения
int numsurfs = 0;
int unitflag = 0;

// Структура для хранения данных системы
static SystemData system_data = {0};

// Функции
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
void WaitForData(HWND hwnd);
void PostRequestMessage(char* szItem, HWND hwndServer, HWND hwnd);
DWORD WINAPI DDEMessageThread(LPVOID lpParam);
char *GetString(char *szBuffer, int n, char *szSubString);

// Функция для извлечения подстроки
char *GetString(char *szBuffer, int n, char *szSubString) {
    char *p = szBuffer;
    int count = 0;

    szSubString[0] = '\0';
    while (count < n) {
        while (*p && *p != ',') p++;
        if (*p == ',') p++;
        count++;
    }

    while (*p == ' ') p++;
    int i = 0;
    while (*p && *p != ',' && *p != '\n') {
        szSubString[i++] = *p++;
    }
    szSubString[i] = '\0';
    return szSubString;
}

// Получение данных о системе
int get_system_data() {
    if (!hwndServer) return 0;

    // Сбрасываем данные
    ResetSystemData();

    // Обновляем линзу
    PostRequestMessage("GetRefresh", hwndServer, hwndServer);
    if (!GotData) {
        AddDebugLog("Failed to refresh system");
        return 0;
    }

    // Получаем имя линзы
    szGlobalBuffer[0] = '\0';
    GotData = 0;
    PostRequestMessage("GetName", hwndServer, hwndServer);
    if (GotData) {
        strncpy(system_data.lensname, szGlobalBuffer, sizeof(system_data.lensname) - 1);
        system_data.lensname[sizeof(system_data.lensname) - 1] = '\0';
        if (strlen(system_data.lensname) >= 2) system_data.lensname[strlen(system_data.lensname) - 2] = '\0'; // Убираем CR-LF
        AddDebugLog("Lens name received");
    } else {
        AddDebugLog("Failed to get lens name");
    }

    // Получаем имя файла
    szGlobalBuffer[0] = '\0';
    GotData = 0;
    PostRequestMessage("GetFile", hwndServer, hwndServer);
    if (GotData) {
        strncpy(system_data.filename, szGlobalBuffer, sizeof(system_data.filename) - 1);
        system_data.filename[sizeof(system_data.filename) - 1] = '\0';
        if (strlen(system_data.filename) >= 2) system_data.filename[strlen(system_data.filename) - 2] = '\0';
        AddDebugLog("File name received");
    } else {
        AddDebugLog("Failed to get file name");
    }

    // Получаем данные системы
    szGlobalBuffer[0] = '\0';
    GotData = 0;
    PostRequestMessage("GetSystem", hwndServer, hwndServer);
    if (GotData) {
        char szSub[256];
        system_data.numsurfs = atoi(GetString(szGlobalBuffer, 0, szSub));
        system_data.units = atoi(GetString(szGlobalBuffer, 1, szSub));
        numsurfs = system_data.numsurfs;
        unitflag = system_data.units;
        AddDebugLog("System data received");
        return 1;
    } else {
        AddDebugLog("Failed to get system data");
        return 0;
    }
}

// Инициализация DDE
int initialize_dde(HWND hwnd) {
    if (hwndServer) return 1;

    ATOM aApp = GlobalAddAtom("ZEMAX");
    ATOM aTop = GlobalAddAtom("System");
    if (!aApp || !aTop) {
        AddDebugLog("Failed to create atoms");
        return 0;
    }

    AddDebugLog("Sending WM_DDE_INITIATE");
    SendMessageTimeout(HWND_BROADCAST, WM_DDE_INITIATE, (WPARAM)hwnd, MAKELONG(aApp, aTop), 
                       SMTO_NORMAL, DDE_TIMEOUT, NULL);

    GlobalDeleteAtom(aApp);
    GlobalDeleteAtom(aTop);

    // Ждём ответа на инициализацию
    MSG msg;
    DWORD startTime = GetTickCount();
    while (GetTickCount() - startTime < DDE_TIMEOUT && !hwndServer) {
        if (PeekMessage(&msg, hwnd, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
            char log[512];
            snprintf(log, sizeof(log), "Received DDE message: %u", msg.message);
            AddDebugLog(log);
            DispatchMessage(&msg);
        }
        Sleep(10);
    }

    if (!hwndServer) {
        AddDebugLog("No DDE server response");
        return 0;
    }

    char log[512];
    snprintf(log, sizeof(log), "DDE server connected: %p", hwndServer);
    AddDebugLog(log);

    // Получаем данные о системе
    return get_system_data();
}

// Отправка запроса в Zemax
const char* send_zemax_request(const char* item) {
    static char result[256] = "";
    char request[256];
    snprintf(request, sizeof(request), "GetSurfaceData,%s,2", item);

    if (!hwndServer) {
        strcpy(result, "DDE not initialized");
        return result;
    }

    char log[512];
    snprintf(log, sizeof(log), "Sending request: %s", request);
    AddDebugLog(log);

    szGlobalBuffer[0] = '\0';
    GotData = 0;
    PostRequestMessage(request, hwndServer, hwndServer);

    if (GotData) {
        snprintf(log, sizeof(log), "Received data: %s", szGlobalBuffer);
        AddDebugLog(log);
        strcpy(result, szGlobalBuffer);
    } else {
        AddDebugLog("Failed to get data");
        strcpy(result, "Failed to get data");
    }

    return result;
}

// Обработка сообщений
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    static int SystemIsValid = 0;
    ATOM aApp, aTop, aItem;
    DDEACK DdeAck;
    DDEDATA *pDdeData;
    GLOBALHANDLE hDdeData;
    UINT_PTR uiLow, uiHi;
    WORD wStatus;
    char szItem[256], szTest[256];

    switch (iMsg) {
        case WM_DDE_ACK:
            AddDebugLog("WM_DDE_ACK received");
            if (!hwndServer) {
                UnpackDDElParam(WM_DDE_ACK, lParam, &uiLow, &uiHi);
                FreeDDElParam(WM_DDE_ACK, lParam);
                hwndServer = (HWND)wParam;
                GlobalDeleteAtom((ATOM)uiLow);
                GlobalDeleteAtom((ATOM)uiHi);
                AddDebugLog("DDE server assigned");
            } else {
                UnpackDDElParam(WM_DDE_ACK, lParam, &uiLow, &uiHi);
                FreeDDElParam(WM_DDE_ACK, lParam);
                GlobalDeleteAtom((ATOM)uiLow);
                GlobalDeleteAtom((ATOM)uiHi);
            }
            return 0;

        case WM_DDE_DATA:
            AddDebugLog("WM_DDE_DATA received");
            UnpackDDElParam(WM_DDE_DATA, lParam, &uiLow, &uiHi);
            FreeDDElParam(WM_DDE_DATA, lParam);
            hDdeData = (GLOBALHANDLE)uiLow;
            pDdeData = (DDEDATA *)GlobalLock(hDdeData);
            aItem = (ATOM)uiHi;

            DdeAck.bAppReturnCode = 0;
            DdeAck.reserved = 0;
            DdeAck.fBusy = FALSE;
            DdeAck.fAck = FALSE;

            if (pDdeData->cfFormat == CF_TEXT) {
                GlobalGetAtomName(aItem, szItem, sizeof(szItem));
                GetString(szItem, 0, szTest);
                GotData = 1;
                strncpy(szGlobalBuffer, (char *)pDdeData->Value, sizeof(szGlobalBuffer) - 1);
                szGlobalBuffer[sizeof(szGlobalBuffer) - 1] = '\0';

                if (strcmp(szTest, "GetName") == 0) {
                    strncpy(system_data.lensname, szGlobalBuffer, sizeof(system_data.lensname) - 1);
                    system_data.lensname[sizeof(system_data.lensname) - 1] = '\0';
                    if (strlen(system_data.lensname) >= 2) system_data.lensname[strlen(system_data.lensname) - 2] = '\0';
                    DdeAck.fAck = TRUE;
                } else if (strcmp(szTest, "GetFile") == 0) {
                    strncpy(system_data.filename, szGlobalBuffer, sizeof(system_data.filename) - 1);
                    system_data.filename[sizeof(system_data.filename) - 1] = '\0';
                    if (strlen(system_data.filename) >= 2) system_data.filename[strlen(system_data.filename) - 2] = '\0';
                    DdeAck.fAck = TRUE;
                } else if (strcmp(szTest, "GetSystem") == 0) {
                    char szSub[256];
                    system_data.numsurfs = atoi(GetString(szGlobalBuffer, 0, szSub));
                    system_data.units = atoi(GetString(szGlobalBuffer, 1, szSub));
                    numsurfs = system_data.numsurfs;
                    unitflag = system_data.units;
                    DdeAck.fAck = TRUE;
                } else if (strcmp(szTest, "GetSurfaceData") == 0) {
                    DdeAck.fAck = TRUE;
                } else if (strcmp(szTest, "GetRefresh") == 0) {
                    SystemIsValid = atoi((char *)pDdeData->Value) ? 0 : 1;
                    DdeAck.fAck = TRUE;
                }
            }

            if (pDdeData->fAckReq == TRUE) {
                wStatus = *((WORD *)&DdeAck);
                if (!PostMessage((HWND)wParam, WM_DDE_ACK, (WPARAM)hwnd, PackDDElParam(WM_DDE_ACK, wStatus, aItem))) {
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
            AddDebugLog("WM_DDE_TERMINATE received");
            PostMessage(hwndServer, WM_DDE_TERMINATE, (WPARAM)hwnd, 0L);
            hwndServer = NULL;
            return 0;

        case WM_DESTROY:
            AddDebugLog("WM_DESTROY received");
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
        AddDebugLog("Failed to post DDE request");
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

// Функции для доступа к данным
SystemData* GetSystemData() {
    return &system_data;
}

void ResetSystemData() {
    memset(&system_data, 0, sizeof(SystemData));
    numsurfs = 0;
    unitflag = 0;
}
