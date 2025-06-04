#include "zemax_dde.h"
#include <windows.h>
#include <ddeml.h>
#include <string>

std::string send_z_surface(const char* surface, const char* sampling, const char* path) {
    DWORD idInst = 0;
    UINT initResult = DdeInitialize(&idInst, NULL, APPCMD_CLIENTONLY, 0);
    if (initResult != DMLERR_NO_ERROR) {
        return "Failed to initialize DDE";
    }

    HSZ hszService = DdeCreateStringHandle(idInst, "ZEMAX", CP_WINANSI);
    HSZ hszTopic = DdeCreateStringHandle(idInst, "SYSTEM", CP_WINANSI);
    HCONV hConv = DdeConnect(idInst, hszService, hszTopic, NULL);
    if (!hConv) {
        DdeFreeStringHandle(idInst, hszService);
        DdeFreeStringHandle(idInst, hszTopic);
        DdeUninitialize(idInst);
        return "Failed to connect to Zemax";
    }

    std::string command = std::string("GetConfig,Surf:") + surface + ",Radius";
    HSZ hszItem = DdeCreateStringHandle(idInst, command.c_str(), CP_WINANSI);
    HDDEDATA hData = DdeClientTransaction(NULL, 0, hConv, hszItem, CF_TEXT, XTYP_REQUEST, 1000, NULL);
    std::string result;

    if (hData) {
        char buffer[256] = {0};
        DdeGetData(hData, (LPBYTE)buffer, sizeof(buffer) - 1, 0);
        result = buffer;
        DdeFreeDataHandle(hData);
    } else {
        result = "Failed to get radius";
    }

    DdeFreeStringHandle(idInst, hszItem);
    DdeDisconnect(hConv);
    DdeFreeStringHandle(idInst, hszService);
    DdeFreeStringHandle(idInst, hszTopic);
    DdeUninitialize(idInst);
    return result;
}