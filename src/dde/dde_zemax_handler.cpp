#include <stdexcept>
#include <string>
#include <vector>
#include <dde.h>
#include "dde_zemax_handler.h"

namespace ZemaxDDE {
    Logger logger;
    OpticalSystemData opticalSystem;

    HWND ZemaxDDEServer = NULL;
    bool IsDataReceived = false;
    std::string BufferString;
    char RequestItemBuffer[256] = {0};

    /*
    * Splits the input string into individual "tokens" (meaningful parts).
    * Tokens are delimited by commas, spaces, newlines, or carriage returns, unless they are enclosed within double quotes.
    * Returns a vector of strings, where each string is a token.
    */
    std::vector<std::string> tokenize(const std::string& bufferStr) {
        std::vector<std::string> tokens;
        std::string currentToken;
        bool inQuotes = false;

        for (char c : bufferStr) {
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if ((c == ',' || c == '\n' || c == '\r' || c == ' ') && !inQuotes) {
                if (!currentToken.empty()) {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
            } else {
                currentToken += c;
            }
        }

        if (!currentToken.empty()) {
            tokens.push_back(currentToken);
        }
        
        return tokens;
    }

    void WaitForData(HWND hwnd) {
        MSG msg;
        DWORD dwTime = GetTickCount();
        IsDataReceived = false;
        while ((GetTickCount() - dwTime < DDE_TIMEOUT_MS) && !IsDataReceived) {
            while (PeekMessage(&msg, hwnd, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
                DispatchMessage(&msg);
            }
        }
    }

    void PostRequestMessage(const char* item, HWND ZemaxDDEServer, HWND hwnd) {
        wchar_t wItem[256];
        MultiByteToWideChar(CP_ACP, 0, item, -1, wItem, 256);
        ATOM aItem = GlobalAddAtomW(wItem);
        if (!PostMessageW(ZemaxDDEServer, WM_DDE_REQUEST, (WPARAM)hwnd, PackDDElParam(WM_DDE_REQUEST, CF_TEXT, aItem))) {
            GlobalDeleteAtom(aItem);
            throw std::runtime_error("Cannot communicate with Zemax");
        }
        WaitForData(hwnd);
    }

    void initiateDDE(HWND hwnd) {
        ZemaxDDEServer = NULL;
        ATOM aApp = GlobalAddAtomW(DDE_APP_NAME);
        ATOM aTop = GlobalAddAtomW(DDE_TOPIC);
        SendMessageW(HWND_BROADCAST, WM_DDE_INITIATE, (WPARAM)hwnd, MAKELONG(aApp, aTop));
        logger.addLog("Sent WM_DDE_INITIATE with app=ZEMAX, topic=RayData");
        GlobalDeleteAtom(aApp);
        GlobalDeleteAtom(aTop);
        if (!ZemaxDDEServer) {
            logger.addLog("No ZemaxDDEServer received");
            throw std::runtime_error("Failed to establish DDE connection with Zemax");
        }
    }

    double getSurfaceRadius(HWND hwnd, int surfaceNumber) {
        if (!ZemaxDDEServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        IsDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetSurfaceData,%d,2", surfaceNumber);
        logger.addLog(std::string("Sending request: ") + request);
        PostRequestMessage(request, ZemaxDDEServer, hwnd);

        if (!IsDataReceived) {
            logger.addLog("No response from Zemax");
            throw std::runtime_error("No response from Zemax");
        }

        logger.addLog("Raw response: " + BufferString);
        
        // BufferString уже std::string, передаём её напрямую
        std::vector<std::string> tokens = tokenize(BufferString);

        double curvature = 0.0;
        if (!tokens.empty()) {
            try {
                curvature = std::stod(tokens[0]);
            } catch (const std::invalid_argument& e) {
                logger.addLog("Error converting token to double: " + std::string(e.what()));
                throw std::runtime_error("Invalid data received for curvature.");
            } catch (const std::out_of_range& e) {
                logger.addLog("Error converting token to double (out of range): " + std::string(e.what()));
                throw std::runtime_error("Curvature value out of range.");
            }
        } else {
            logger.addLog("No tokens found in response for GetSurfaceData.");
            throw std::runtime_error("Unexpected response format for GetSurfaceData.");
        }

        double radius = (curvature != 0.0) ? 1.0 / curvature : 0.0;
        logger.addLog("Calculated radius: " + std::to_string(radius));

        return radius;
    }

    void getLensName(HWND hwnd) {
        if (!ZemaxDDEServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        IsDataReceived = false;
        logger.addLog("Sending request: GetName");
        PostRequestMessage("GetName", ZemaxDDEServer, hwnd);

        if (!IsDataReceived) {
            logger.addLog("No response from Zemax for GetName");
            throw std::runtime_error("No response from Zemax for GetName");
        }

        logger.addLog(std::string("Raw response for GetName: ") + BufferString);
        
        // Присваиваем std::string напрямую, затем работаем с char[] для OpticalSystemData
        std::string temp_lens_name = BufferString; 
        strncpy(opticalSystem.lensName, temp_lens_name.c_str(), sizeof(opticalSystem.lensName) - 1);
        opticalSystem.lensName[sizeof(opticalSystem.lensName) - 1] = '\0';

        size_t len = strlen(opticalSystem.lensName);
        while (len > 0 && (opticalSystem.lensName[len - 1] == '\n' || opticalSystem.lensName[len - 1] == '\r')) {
            opticalSystem.lensName[len - 1] = '\0';
            len--;
        }
        
        logger.addLog(std::string("Parsed Lens Name: ") + opticalSystem.lensName);
    }

    void getFileName(HWND hwnd) {
        if (!ZemaxDDEServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        IsDataReceived = false;
        logger.addLog("Sending request: GetFile");
        PostRequestMessage("GetFile", ZemaxDDEServer, hwnd);

        if (!IsDataReceived) {
            logger.addLog("No response from Zemax for GetFile");
            throw std::runtime_error("No response from Zemax for GetFile");
        }

        logger.addLog(std::string("Raw response for GetFile: ") + BufferString);
        
        // Присваиваем std::string напрямую, затем работаем с char[] для OpticalSystemData
        std::string temp_file_name = BufferString;
        strncpy(opticalSystem.fileName, temp_file_name.c_str(), sizeof(opticalSystem.fileName) - 1);
        opticalSystem.fileName[sizeof(opticalSystem.fileName) - 1] = '\0';

        size_t len = strlen(opticalSystem.fileName);
        while (len > 0 && (opticalSystem.fileName[len - 1] == '\n' || opticalSystem.fileName[len - 1] == '\r')) {
            opticalSystem.fileName[len - 1] = '\0';
            len--;
        }
        logger.addLog(std::string("Parsed File Name: ") + opticalSystem.fileName);
    }

    void getSystemData(HWND hwnd) {
        if (!ZemaxDDEServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        IsDataReceived = false;
        logger.addLog("Sending request: GetSystem");
        PostRequestMessage("GetSystem", ZemaxDDEServer, hwnd);

        if (!IsDataReceived) {
            logger.addLog("No response from Zemax for GetSystem");
            throw std::runtime_error("No response from Zemax for GetSystem");
        }

        logger.addLog(std::string("Raw response for GetSystem: ") + BufferString);
        
        // BufferString уже std::string, передаём её напрямую
        std::vector<std::string> tokens = tokenize(BufferString);

        if (tokens.size() >= 2) {
            try {
                opticalSystem.numSurfs = std::stoi(tokens[0]);
                opticalSystem.units = std::stoi(tokens[1]);
                logger.addLog("Parsed numSurfs: " + std::to_string(opticalSystem.numSurfs));
                logger.addLog("Parsed units: " + std::to_string(opticalSystem.units));
            } catch (const std::invalid_argument& e) {
                logger.addLog("Error converting token to int: " + std::string(e.what()));
                throw std::runtime_error("Invalid data received for system parameters.");
            } catch (const std::out_of_range& e) {
                logger.addLog("Error converting token to int (out of range): " + std::string(e.what()));
                throw std::runtime_error("System parameters values out of range.");
            }
        } else {
            logger.addLog("Not enough tokens in response for GetSystem.");
            throw std::runtime_error("Unexpected response format for GetSystem.");
        }
    }

    void getFieldData(HWND hwnd, int fieldIndex) {
        if (!ZemaxDDEServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        IsDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetField,%d", fieldIndex);
        logger.addLog(std::string("Sending request: ") + request);
        PostRequestMessage(request, ZemaxDDEServer, hwnd);

        if (!IsDataReceived) {
            logger.addLog("No response from Zemax for GetField");
            throw std::runtime_error("No response from Zemax for GetField");
        }

        logger.addLog(std::string("Raw response for GetField: ") + BufferString);
        
        // BufferString уже std::string, передаём её напрямую
        std::vector<std::string> tokens = tokenize(BufferString);

        if (fieldIndex == 0) {
            if (tokens.size() >= 2) {
                try {
                    opticalSystem.fieldType = std::stoi(tokens[0]);
                    opticalSystem.numFields = std::stoi(tokens[1]);
                    logger.addLog("Parsed fieldType: " + std::to_string(opticalSystem.fieldType));
                    logger.addLog("Parsed numFields: " + std::to_string(opticalSystem.numFields));
                } catch (const std::invalid_argument& e) {
                    logger.addLog("Error converting token to int: " + std::string(e.what()));
                    throw std::runtime_error("Invalid data received for field parameters (index 0).");
                } catch (const std::out_of_range& e) {
                    logger.addLog("Error converting token to int (out of range): " + std::string(e.what()));
                    throw std::runtime_error("Field parameters values out of range (index 0).");
                }
            } else {
                logger.addLog("Not enough tokens in response for GetField (index 0).");
                throw std::runtime_error("Unexpected response format for GetField (index 0).");
            }
        } else if (fieldIndex >= 1 && fieldIndex <= 11) {
            if (tokens.size() >= 2) {
                try {
                    opticalSystem.xField[fieldIndex] = std::stod(tokens[0]);
                    opticalSystem.yField[fieldIndex] = std::stod(tokens[1]);
                    logger.addLog("Parsed xField[" + std::to_string(fieldIndex) + "]: " + std::to_string(opticalSystem.xField[fieldIndex]));
                    logger.addLog("Parsed yField[" + std::to_string(fieldIndex) + "]: " + std::to_string(opticalSystem.yField[fieldIndex]));
                } catch (const std::invalid_argument& e) {
                    logger.addLog("Error converting token to double: " + std::string(e.what()));
                    throw std::runtime_error("Invalid data received for field coordinates.");
                } catch (const std::out_of_range& e) {
                    logger.addLog("Error converting token to double (out of range): " + std::string(e.what()));
                    throw std::runtime_error("Field coordinates values out of range.");
                }
            } else {
                logger.addLog("Not enough tokens in response for GetField (index > 0).");
                throw std::runtime_error("Unexpected response format for GetField (index > 0).");
            }
        } else {
            logger.addLog("Invalid fieldIndex: " + std::to_string(fieldIndex));
            throw std::runtime_error("Invalid field index provided.");
        }
    }

    void getWaveData(HWND hwnd, int waveIndex) {
        if (!ZemaxDDEServer) {
            throw std::runtime_error("DDE connection to Zemax not established");
        }

        IsDataReceived = false;
        char request[256];
        snprintf(request, sizeof(request), "GetWave,%d", waveIndex);
        logger.addLog(std::string("Sending request: ") + request);
        PostRequestMessage(request, ZemaxDDEServer, hwnd);

        if (!IsDataReceived) {
            logger.addLog("No response from Zemax for GetWave");
            throw std::runtime_error("No response from Zemax for GetWave");
        }

        logger.addLog(std::string("Raw response for GetWave: ") + BufferString);
        
        // BufferString уже std::string, передаём её напрямую
        std::vector<std::string> tokens = tokenize(BufferString);

        if (waveIndex == 0) {
            if (tokens.size() >= 2) {
                try {
                    opticalSystem.primWave = std::stoi(tokens[0]);
                    opticalSystem.numWaves = std::stoi(tokens[1]);
                    logger.addLog("Parsed primWave: " + std::to_string(opticalSystem.primWave));
                    logger.addLog("Parsed numWaves: " + std::to_string(opticalSystem.numWaves));
                } catch (const std::invalid_argument& e) {
                    logger.addLog("Error converting token to int: " + std::string(e.what()));
                    throw std::runtime_error("Invalid data received for wave parameters (index 0).");
                } catch (const std::out_of_range& e) {
                    logger.addLog("Error converting token to int (out of range): " + std::string(e.what()));
                    throw std::runtime_error("Wave parameters values out of range (index 0).");
                }
            } else {
                logger.addLog("Not enough tokens in response for GetWave (index 0).");
                throw std::runtime_error("Unexpected response format for GetWave (index 0).");
            }
        } else if (waveIndex >= 1 && waveIndex <= 23) {
            if (!tokens.empty()) {
                try {
                    opticalSystem.waveLen[waveIndex] = std::stod(tokens[0]);
                    logger.addLog("Parsed waveLen[" + std::to_string(waveIndex) + "]: " + std::to_string(opticalSystem.waveLen[waveIndex]));
                } catch (const std::invalid_argument& e) {
                    logger.addLog("Error converting token to double: " + std::string(e.what()));
                    throw std::runtime_error("Invalid data received for wavelength.");
                } catch (const std::out_of_range& e) {
                    logger.addLog("Error converting token to double (out of range): " + std::string(e.what()));
                    throw std::runtime_error("Wavelength value out of range.");
                }
            } else {
                logger.addLog("No tokens found in response for GetWave (index > 0).");
                throw std::runtime_error("Unexpected response format for GetWave (index > 0).");
            }
        } else {
            logger.addLog("Invalid waveIndex: " + std::to_string(waveIndex));
            throw std::runtime_error("Invalid wave index provided.");
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
                if (!ZemaxDDEServer) {
                    UnpackDDElParam(WM_DDE_ACK, lParam, &uiLow, &uiHi);
                    FreeDDElParam(WM_DDE_ACK, lParam);
                    ZemaxDDEServer = (HWND)wParam;
                    logger.addLog("DDE_ACK received, ZemaxDDEServer set to: " + std::to_string((uintptr_t)ZemaxDDEServer));
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
                    WideCharToMultiByte(CP_ACP, 0, wItem, -1, RequestItemBuffer, sizeof(RequestItemBuffer), NULL, NULL);
                    
                    std::string dde_item_str(RequestItemBuffer);
                    std::vector<std::string> item_tokens = tokenize(dde_item_str);
                    
                    std::string command_token = "";
                    if (!item_tokens.empty()) {
                        command_token = item_tokens[0];
                    }

                    IsDataReceived = true;
                    // !!! ВАЖНОЕ ИЗМЕНЕНИЕ ЗДЕСЬ !!!
                    // Присваиваем данные напрямую в std::string BufferString.
                    // pDdeData->Value это char*, std::string конструктор безопасно его скопирует
                    // до первого null-терминатора.
                    BufferString = (char*)pDdeData->Value; 
                    logger.addLog("DDE_DATA received, content: " + BufferString);

                    if (command_token == "GetSurfaceData") {
                        DdeAck.fAck = TRUE;
                    }
                    if (command_token == "GetName") {
                        // Дублирование логики, но теперь с std::string в качестве источника
                        strncpy(opticalSystem.lensName, BufferString.c_str(), sizeof(opticalSystem.lensName) - 1);
                        opticalSystem.lensName[sizeof(opticalSystem.lensName) - 1] = '\0';
                        size_t len = strlen(opticalSystem.lensName);
                        while (len > 0 && (opticalSystem.lensName[len - 1] == '\n' || opticalSystem.lensName[len - 1] == '\r')) {
                            opticalSystem.lensName[len - 1] = '\0';
                            len--;
                        }
                        DdeAck.fAck = TRUE;
                    }
                    if (command_token == "GetFile") {
                        // Дублирование логики, но теперь с std::string в качестве источника
                        strncpy(opticalSystem.fileName, BufferString.c_str(), sizeof(opticalSystem.fileName) - 1);
                        opticalSystem.fileName[sizeof(opticalSystem.fileName) - 1] = '\0';
                        size_t len = strlen(opticalSystem.fileName);
                        while (len > 0 && (opticalSystem.fileName[len - 1] == '\n' || opticalSystem.fileName[len - 1] == '\r')) {
                            opticalSystem.fileName[len - 1] = '\0';
                            len--;
                        }
                        DdeAck.fAck = TRUE;
                    }
                    if (command_token == "GetSystem") {
                        std::vector<std::string> buffer_tokens = tokenize(BufferString);
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

                        std::vector<std::string> buffer_tokens = tokenize(BufferString);
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

                        std::vector<std::string> buffer_tokens = tokenize(BufferString);
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
                // WINUSERAPI WINBOOL WINAPI PostMessageW (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
                PostMessageW(ZemaxDDEServer, WM_DDE_TERMINATE, (WPARAM)hwnd, 0L);
                ZemaxDDEServer = NULL;
                logger.addLog("DDE connection terminated");
                return 0;
        }
        return 0;
    }

    void terminateDDE() {
        if (ZemaxDDEServer) {
            PostMessageW(ZemaxDDEServer, WM_DDE_TERMINATE, 0, 0);
            ZemaxDDEServer = NULL;
            logger.addLog("DDE connection terminated");
        }
    }
}
