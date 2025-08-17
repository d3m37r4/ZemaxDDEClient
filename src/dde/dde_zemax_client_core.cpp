#include <stdexcept>
#include <dde.h>
#include "dde_zemax_client.h"
#include "dde_zemax_utils.h"

namespace ZemaxDDE {
    ZemaxDDEClient::ZemaxDDEClient(HWND zemaxDDEClient) : zemaxDDEClient(zemaxDDEClient) {}

    ZemaxDDEClient::~ZemaxDDEClient() {
        terminateDDE();
    }

    void ZemaxDDEClient::initiateDDE() {
        zemaxDDEServer = NULL;
        ATOM aApp = GlobalAddAtomW(DDE_APP_NAME);
        ATOM aTop = GlobalAddAtomW(DDE_TOPIC);
        SendMessageW(HWND_BROADCAST, WM_DDE_INITIATE, (WPARAM)zemaxDDEClient, MAKELONG(aApp, aTop));
#ifdef DEBUG_LOG
        char appName[256];
        char topicName[256];
        WideCharToMultiByte(CP_ACP, 0, DDE_APP_NAME, -1, appName, sizeof(appName), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, DDE_TOPIC, -1, topicName, sizeof(topicName), NULL, NULL);
        logger.addLog("Sent 'WM_DDE_INITIATE' with app = " + std::string(appName) + ", topic = " + std::string(topicName));
#endif
        GlobalDeleteAtom(aApp);
        GlobalDeleteAtom(aTop);

        checkDDEConnection();
#ifdef DEBUG_LOG
        logger.addLog("DDE connection established successfully");
#endif
    }

    void ZemaxDDEClient::terminateDDE() {
        if (zemaxDDEServer) {
            PostMessageW(zemaxDDEServer, WM_DDE_TERMINATE, (WPARAM)zemaxDDEClient, 0L);
            zemaxDDEServer = NULL;
#ifdef DEBUG_LOG
            logger.addLog("DDE connection terminated");
#endif
        }
    }

    void ZemaxDDEClient::waitForData() {
        MSG msg;
        DWORD dwTime = GetTickCount();
        isDataReceived = false;
        while ((GetTickCount() - dwTime < DDE_TIMEOUT_MS) && !isDataReceived) {
            while (PeekMessage(&msg, zemaxDDEClient, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
                DispatchMessage(&msg);
            }
        }
    }

    void ZemaxDDEClient::sendPostRequest(const char* request) {
#ifdef DEBUG_LOG
        logger.addLog(std::string("Sending request = ") + request);
#endif
        wchar_t wItem[256];
        MultiByteToWideChar(CP_ACP, 0, request, -1, wItem, 256);
        ATOM aItem = GlobalAddAtomW(wItem);
        if (!PostMessageW(zemaxDDEServer, WM_DDE_REQUEST, (WPARAM)zemaxDDEClient, PackDDElParam(WM_DDE_REQUEST, CF_TEXT, aItem))) {
            GlobalDeleteAtom(aItem);
            throw std::runtime_error("Cannot communicate with Zemax");
        }
        waitForData();
    }

    LRESULT ZemaxDDEClient::handleDDEMessages(UINT iMsg, WPARAM wParam, LPARAM lParam) {
        static DDEACK DdeAck;
        GLOBALHANDLE hDdeData;
        DDEDATA* pDdeData;
        ATOM aItem;
        UINT_PTR uiLow, uiHi;
#ifdef DEBUG_LOG
        logger.addLog("Received DDE message = " + std::to_string(iMsg));
#endif
        switch (iMsg) {
            case WM_DDE_ACK: {
                if (!zemaxDDEServer) {
                    UnpackDDElParam(WM_DDE_ACK, lParam, &uiLow, &uiHi);
                    FreeDDElParam(WM_DDE_ACK, lParam);
                    zemaxDDEServer = (HWND)wParam;
#ifdef DEBUG_LOG 
                    logger.addLog("Message 'DDE_ACK' received, ZemaxDDEServer = " + std::to_string((uintptr_t)zemaxDDEServer));
#endif
                    GlobalDeleteAtom((ATOM)uiLow);
                    GlobalDeleteAtom((ATOM)uiHi);
                }
                return 0;
            }
            case WM_DDE_DATA: {
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
                    WideCharToMultiByte(CP_ACP, 0, wItem, -1, requestItemBuffer, sizeof(requestItemBuffer), NULL, NULL);

                    std::string dde_item_str(requestItemBuffer);
                    std::vector<std::string> item_tokens = ZemaxDDE::tokenize(dde_item_str);

                    std::string command_token = "";
                    if (!item_tokens.empty()) {
                        command_token = item_tokens[0];
                    }

                    isDataReceived = true;
                    bufferString = (char*)pDdeData->Value;
                    logger.addLog("Message 'DDE_DATA' received, content = " + bufferString);

                    if (command_token == "GetSurfaceData") {
                        DdeAck.fAck = TRUE;
                    }
                    if (command_token == "GetName") {
                        opticalSystem.lensName = bufferString;
                        DdeAck.fAck = TRUE;
                    }
                    if (command_token == "GetFile") {
                        opticalSystem.fileName = bufferString;
                        DdeAck.fAck = TRUE;
                    }
                    if (command_token == "GetSystem") {
                        std::vector<std::string> buffer_tokens = ZemaxDDE::tokenize(bufferString);
                        if (buffer_tokens.size() >= 2) {
                            try {
                                opticalSystem.numSurfs = std::stoi(buffer_tokens[0]);
                                opticalSystem.units = std::stoi(buffer_tokens[1]);
                                DdeAck.fAck = TRUE;
                            } catch (const std::invalid_argument& e) {
                                logger.addLog("Error converting token to int (GetSystem in handleDDEMessages): " + std::string(e.what()));
                            } catch (const std::out_of_range& e) {
                                logger.addLog("Error converting token to int (GetSystem in handleDDEMessages, out of range): " + std::string(e.what()));
                            }
                        } else {
                            logger.addLog("Not enough tokens for GetSystem in handleDDEMessages.");
                        }
                    }
                    if (command_token == "GetField") {
                        int fn = -1;
                        if (item_tokens.size() >= 2) {
                            try {
                                fn = std::stoi(item_tokens[1]);
                            } catch (const std::invalid_argument& e) {
                                logger.addLog("Error converting field index token to int: " + std::string(e.what()));
                            } catch (const std::out_of_range& e) {
                                logger.addLog("Error converting field index token to int (out of range): " + std::string(e.what()));
                            }
                        }

                        std::vector<std::string> buffer_tokens = ZemaxDDE::tokenize(bufferString);
                        if (fn == 0) {
                            if (buffer_tokens.size() >= 2) {
                                try {
                                    opticalSystem.fieldType = std::stoi(buffer_tokens[0]);
                                    opticalSystem.numFields = std::stoi(buffer_tokens[1]);
                                    DdeAck.fAck = TRUE;
                                } catch (const std::invalid_argument& e) {
                                    logger.addLog("Error converting field data token to int (index 0): " + std::string(e.what()));
                                } catch (const std::out_of_range& e) {
                                    logger.addLog("Error converting field data token to int (index 0, out of range): " + std::string(e.what()));
                                }
                            } else {
                                logger.addLog("Not enough tokens for GetField (index 0) in handleDDEMessages.");
                            }
                        } else if (fn >= 1 && fn <= 11) {
                            if (buffer_tokens.size() >= 2) {
                                try {
                                    opticalSystem.xField[fn] = std::stod(buffer_tokens[0]);
                                    opticalSystem.yField[fn] = std::stod(buffer_tokens[1]);
                                    DdeAck.fAck = TRUE;
                                } catch (const std::invalid_argument& e) {
                                    logger.addLog("Error converting field data token to double: " + std::string(e.what()));
                                } catch (const std::out_of_range& e) {
                                    logger.addLog("Error converting field data token to double (out of range): " + std::string(e.what()));
                                }
                            } else {
                                logger.addLog("Not enough tokens for GetField (index > 0) in handleDDEMessages.");
                            }
                        }
                    }
                    if (command_token == "GetWave") {
                        int wn = -1;
                        if (item_tokens.size() >= 2) {
                            try {
                                wn = std::stoi(item_tokens[1]);
                            } catch (const std::invalid_argument& e) {
                                logger.addLog("Error converting wave index token to int: " + std::string(e.what()));
                            } catch (const std::out_of_range& e) {
                                logger.addLog("Error converting wave index token to int (out of range): " + std::string(e.what()));
                            }
                        }

                        std::vector<std::string> buffer_tokens = ZemaxDDE::tokenize(bufferString);
                        if (wn == 0) {
                            if (buffer_tokens.size() >= 2) {
                                try {
                                    opticalSystem.primWave = std::stoi(buffer_tokens[0]);
                                    opticalSystem.numWaves = std::stoi(buffer_tokens[1]);
                                    DdeAck.fAck = TRUE;
                                } catch (const std::invalid_argument& e) {
                                    logger.addLog("Error converting wave data token to int (index 0): " + std::string(e.what()));
                                } catch (const std::out_of_range& e) {
                                    logger.addLog("Error converting wave data token to int (index 0, out of range): " + std::string(e.what()));
                                }
                            } else {
                                logger.addLog("Not enough tokens for GetWave (index 0) in handleDDEMessages.");
                            }
                        } else if (wn >= 1 && wn <= 23) {
                            if (!buffer_tokens.empty()) {
                                try {
                                    opticalSystem.waveLen[wn] = std::stod(buffer_tokens[0]);
                                    DdeAck.fAck = TRUE;
                                } catch (const std::invalid_argument& e) {
                                    logger.addLog("Error converting wave data token to double: " + std::string(e.what()));
                                } catch (const std::out_of_range& e) {
                                    logger.addLog("Error converting wave data token to double (out of range): " + std::string(e.what()));
                                }
                            } else {
                                logger.addLog("No tokens for GetWave (index > 0) in handleDDEMessages.");
                            }
                        }
                    }
                }

                if (pDdeData->fAckReq == TRUE) {
                    WORD wStatus = *((WORD*)&DdeAck);
                    if (!PostMessageW((HWND)wParam, WM_DDE_ACK, (WPARAM)zemaxDDEClient, PackDDElParam(WM_DDE_ACK, wStatus, aItem))) {
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
            }
        }
        return 0;
    }

    void ZemaxDDEClient::checkDDEConnection() {
        const char* const DDE_ERROR_MSG_CONNECTION_NOT_ESTABLISHED = "No ZemaxDDEServer received, DDE connection to Zemax not established";
        if (!zemaxDDEServer) {
#ifdef DEBUG_LOG
            logger.addLog(DDE_ERROR_MSG_CONNECTION_NOT_ESTABLISHED);
#endif
            throw std::runtime_error(DDE_ERROR_MSG_CONNECTION_NOT_ESTABLISHED);
        }
    }

    void ZemaxDDEClient::checkResponseStatus(const std::string& errorMsg) {
        if (!isDataReceived) {
#ifdef DEBUG_LOG
            logger.addLog(errorMsg);
#endif
            throw std::runtime_error(errorMsg);
        }
    }
}
