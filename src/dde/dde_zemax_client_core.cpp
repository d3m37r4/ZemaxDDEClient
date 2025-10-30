#include <stdexcept>
#include <dde.h>
#include "dde_zemax_client.h"
#include "dde_zemax_utils.h"

namespace ZemaxDDE {
    ZemaxDDEClient::ZemaxDDEClient(HWND zemaxDDEClient) : zemaxDDEClient(zemaxDDEClient) {}

    ZemaxDDEClient::~ZemaxDDEClient() {
        terminateDDE();
    }

    void ZemaxDDEClient::setOnDDEConnectedCallback(OnDDEConnectedCallback callback) {
        onDDEConnected = callback;
    }

    void ZemaxDDEClient::initiateDDE() {
        if (zemaxDDEServer != NULL) {
            logger.addLog("[DDE] DDE already connected. Skipping initiate.");
            return;
        }

        zemaxDDEServer = NULL;

        ATOM aApp = GlobalAddAtomW(DDE_APP_NAME);
        ATOM aTop = GlobalAddAtomW(DDE_TOPIC);
        SendMessageW(HWND_BROADCAST, WM_DDE_INITIATE, (WPARAM)zemaxDDEClient, MAKELONG(aApp, aTop));

    #ifdef DEBUG_LOG
        char appName[256], topicName[256];
        WideCharToMultiByte(CP_ACP, 0, DDE_APP_NAME, -1, appName, sizeof(appName), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, DDE_TOPIC, -1, topicName, sizeof(topicName), NULL, NULL);
        logger.addLog("[DDE] Sent 'WM_DDE_INITIATE' to app='" + std::string(appName) + "', topic='" + std::string(topicName) + "'");
    #endif
    
        GlobalDeleteAtom(aApp);
        GlobalDeleteAtom(aTop);
        checkDDEConnection();

        if (zemaxDDEServer) {
            logger.addLog("[DDE] Connection established successfully");
        } else {
            logger.addLog("[DDE] Connection not established yet (waiting for 'WM_DDE_ACK')");
        }

        if (onDDEConnected) {
            try {
                onDDEConnected(this);
            } catch (const std::exception& e) {
                logger.addLog("[DDE] Error in DDE connected callback: " + std::string(e.what()));
                throw;
            }
        }
    }

    void ZemaxDDEClient::terminateDDE() {
        if (zemaxDDEServer) {
            PostMessageW(zemaxDDEServer, WM_DDE_TERMINATE, (WPARAM)zemaxDDEClient, 0L);
            zemaxDDEServer = NULL;
            logger.addLog("[DDE] Connection terminated");
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
        logger.addLog("[DDE] Sending request: " + std::string(request));
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
        std::string buffer;
    #ifdef DEBUG_LOG
        logger.addLog("[DDE] Received message: " + std::to_string(iMsg));
    #endif
        switch (iMsg) {
            case WM_DDE_ACK: {
                if (!zemaxDDEServer) {
                    UnpackDDElParam(WM_DDE_ACK, lParam, &uiLow, &uiHi);
                    FreeDDElParam(WM_DDE_ACK, lParam);

                    zemaxDDEServer = (HWND)wParam;

                    GlobalDeleteAtom((ATOM)uiLow);
                    GlobalDeleteAtom((ATOM)uiHi);
                #ifdef DEBUG_LOG
                    logger.addLog("[DDE] Received 'WM_DDE_ACK', zemaxDDEServer = " + std::to_string((uintptr_t)zemaxDDEServer));
                #endif
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
                    char item[512]; 
                    wchar_t wItem[512];
                    GlobalGetAtomNameW(aItem, wItem, sizeof(wItem));
                    WideCharToMultiByte(CP_ACP, 0, wItem, -1, item, sizeof(item), NULL, NULL);

                    std::string dde_item_str(item);
                    std::vector<std::string> item_tokens = ZemaxDDE::tokenize(dde_item_str);
                    std::string command_token = item_tokens.empty() ? "" : item_tokens[0];

                    isDataReceived = true;
                    buffer = (char*)pDdeData->Value;
                #ifdef DEBUG_LOG
                    logger.addLog("[DDE] Received 'WM_DDE_DATA', content = " + buffer);
                #endif
                    if (command_token == "GetName") {
                        opticalSystem.lensName = buffer;
                        DdeAck.fAck = TRUE;
                    }
                    if (command_token == "GetFile") {
                        opticalSystem.fileName = buffer;
                        DdeAck.fAck = TRUE;
                    }
                    if (command_token == "GetSystem") {
                        const int GET_SYSTEM_PARAMS_COUNT = 9;
                        auto systemParams = ZemaxDDE::tokenize(buffer);
                        int params = static_cast<int>(systemParams.size());

                        if (params != GET_SYSTEM_PARAMS_COUNT) {
                            logger.addLog("[DDE] GetSystem: Invalid parameter count. Expected exactly " +
                                        std::to_string(GET_SYSTEM_PARAMS_COUNT) + ", got " +
                                        std::to_string(params));
                            return 0;
                        }

                        try {
                            // GetSystem parameter indexes
                            enum {
                                NUM_SURFS,                      // Number of surfaces
                                UNIT_CODE,                      // Unit code (0=mm, 1=cm, 2=in, 3=M)
                                STOP_SURF,                      // Stop surface number
                                NON_AXIAL_FLAG,                 // Non-axial symmetry flag (0=axial, 1=non-axial)
                                RAY_AIMING_TYPE,                // Ray aiming type (0=off, 1=paraxial, 2=real)
                                ADJUST_INDEX,                   // Index adjustment flag (0=false, 1=true)
                                TEMP,                           // Temperature
                                PRESSURE,                       // Pressure
                                GLOBAL_REF_SURF                 // Global reference surface
                            };

                            opticalSystem.numSurfs      = std::stoi(systemParams[NUM_SURFS]);
                            opticalSystem.units         = std::stoi(systemParams[UNIT_CODE]);
                            opticalSystem.stopSurf      = std::stoi(systemParams[STOP_SURF]);
                            opticalSystem.nonAxialFlag  = std::stoi(systemParams[NON_AXIAL_FLAG]);
                            opticalSystem.rayAimingType = std::stoi(systemParams[RAY_AIMING_TYPE]);
                            opticalSystem.adjustIndex   = std::stoi(systemParams[ADJUST_INDEX]);
                            opticalSystem.temp          = std::stod(systemParams[TEMP]);
                            opticalSystem.pressure      = std::stod(systemParams[PRESSURE]);
                            opticalSystem.globalRefSurf = std::stoi(systemParams[GLOBAL_REF_SURF]);

                            DdeAck.fAck = TRUE;
                        } catch (const std::invalid_argument& e) {
                            logger.addLog("[DDE] GetSystem: Invalid number format in parameter: " + std::string(e.what()));
                            return 0;
                        } catch (const std::out_of_range& e) {
                            logger.addLog("[DDE] GetSystem: Number out of range: " + std::string(e.what()));
                            return 0;
                        } catch (const std::exception& e) {
                            logger.addLog("[DDE] GetSystem: Unexpected error: " + std::string(e.what()));
                            return 0;
                        }
                    }
                    if (command_token == "GetField") {
                        const int EXPECTED_COMMAND_TOKENS = 2;
                        int params = static_cast<int>(item_tokens.size());

                        if (params != EXPECTED_COMMAND_TOKENS) {
                            logger.addLog("[DDE] GetField: Invalid command format. Expected exactly " +
                                        std::to_string(EXPECTED_COMMAND_TOKENS) + " tokens, got " +
                                        std::to_string(params));
                            return 0;
                        }

                        int arg = -1;
                        try {
                            arg = std::stoi(item_tokens[1]);
                        } catch (const std::invalid_argument&) {
                            logger.addLog("[DDE] GetField: Invalid field index '" + item_tokens[1] + "'. Not a number.");
                            return 0;
                        } catch (const std::out_of_range&) {
                            logger.addLog("[DDE] GetField: Field index '" + item_tokens[1] + "' is out of range (too large).");
                            return 0;
                        }

                        if (arg == 0 || (arg >= ZemaxDDE::MIN_FIELDS && arg <= ZemaxDDE::MAX_FIELDS)) {
                            auto tokens = ZemaxDDE::tokenize(buffer);
                            int dataCount = static_cast<int>(tokens.size());

                            if(arg == 0) {
                                const int GET_FIELD_META_COUNT = 5;     // fieldType, numFields, maxX, maxY, normalizationMethod
                                if (dataCount != GET_FIELD_META_COUNT) {
                                    logger.addLog("[DDE] GetField: Invalid parameter count for field metadata. Expected exactly " +
                                                std::to_string(GET_FIELD_META_COUNT) + ", got " +
                                                std::to_string(dataCount));
                                    return 0;
                                }

                                try {
                                    enum  {
                                        FIELD_TYPE,
                                        NUM_FIELDS,
                                        MAX_X_FIELD,
                                        MAX_Y_FIELD,
                                        NORMALIZATION_METHOD
                                    };

                                    opticalSystem.fieldType = std::stoi(tokens[FIELD_TYPE]);

                                    int numFields = std::stoi(tokens[NUM_FIELDS]);
                                    if (numFields < ZemaxDDE::MIN_FIELDS || numFields > ZemaxDDE::MAX_FIELDS) {
                                        logger.addLog("[DDE] GetField: Invalid numFields value: " + std::to_string(numFields) +
                                                    ". Must be in range [" + std::to_string(ZemaxDDE::MIN_FIELDS) + ", " +
                                                    std::to_string(ZemaxDDE::MAX_FIELDS) + "]");
                                        return 0;
                                    }

                                    opticalSystem.numFields = numFields;
                                    opticalSystem.maxXField = std::stod(tokens[MAX_X_FIELD]);
                                    opticalSystem.maxYField = std::stod(tokens[MAX_Y_FIELD]);
                                    opticalSystem.normalizationMethod = std::stoi(tokens[NORMALIZATION_METHOD]);
                                } catch (const std::exception& e) {
                                    logger.addLog("[DDE] GetField: Failed to parse field metadata: " + std::string(e.what()));
                                    return 0;
                                }
                            } else {
                                const int GET_FIELD_DATA_COUNT = 8;     // xField, yField, weight, vDx, vDy, vCx, vCy, vAn
                                if (dataCount != GET_FIELD_DATA_COUNT) {
                                    logger.addLog("[DDE] GetField: Invalid parameter count for field data. Expected exactly " +
                                                std::to_string(GET_FIELD_DATA_COUNT) + ", got " +
                                                std::to_string(dataCount));
                                    return 0;
                                }

                                try {
                                    enum {
                                        XFIELD,
                                        YFIELD,
                                        // WEIGHT,
                                        // VDX,
                                        // VDY,
                                        // VCX,
                                        // VCY,
                                        // VAN
                                    };

                                    opticalSystem.xField[arg] = std::stod(tokens[XFIELD]);
                                    opticalSystem.yField[arg] = std::stod(tokens[YFIELD]);
                                } catch (const std::exception& e) {
                                    logger.addLog("[DDE] GetField: Failed to parse data for field " + std::to_string(arg) +
                                                ": " + std::string(e.what()));
                                    return 0;
                                }
                            }
                        } else {
                            logger.addLog("[DDE] GetField: Field index must be 0 (metadata) or in range [" + 
                                        std::to_string(ZemaxDDE::MIN_FIELDS) + ", " +
                                        std::to_string(ZemaxDDE::MAX_FIELDS) + "]. Got: " +
                                        std::to_string(arg));
                            return 0;
                        }
                        DdeAck.fAck = TRUE;
                    }
                    if (command_token == "GetWave") {
                        const int EXPECTED_COMMAND_TOKENS = 2;
                        int params = static_cast<int>(item_tokens.size());

                        if (params != EXPECTED_COMMAND_TOKENS) {
                            logger.addLog("[DDE] GetWave: Invalid command format. Expected exactly " +
                                        std::to_string(EXPECTED_COMMAND_TOKENS) + " tokens, got " +
                                        std::to_string(params));
                            return 0;
                        }

                        int arg = -1;
                        try {
                            arg = std::stoi(item_tokens[1]);
                        } catch (const std::invalid_argument&) {
                            logger.addLog("[DDE] GetWave: Invalid wave index '" + item_tokens[1] + "'. Not a number.");
                            return 0;
                        } catch (const std::out_of_range&) {
                            logger.addLog("[DDE] GetWave: Wave index '" + item_tokens[1] + "' is out of range (too large).");
                            return 0;
                        }

                        if (arg == 0 || (arg >= ZemaxDDE::MIN_WAVES && arg <= ZemaxDDE::MAX_WAVES)) {
                            auto tokens = ZemaxDDE::tokenize(buffer);
                            int dataCount = static_cast<int>(tokens.size());

                            if(arg == 0) {
                                const int GET_WAVE_META_COUNT = 2;     // primary, number
                                if (dataCount != GET_WAVE_META_COUNT) {
                                    logger.addLog("[DDE] GetWave: Invalid parameter count for wave metadata. Expected exactly " +
                                                std::to_string(GET_WAVE_META_COUNT) + ", got " +
                                                std::to_string(dataCount));
                                    return 0;
                                }

                                try {
                                    enum  {
                                        PRIM_WAVE,
                                        NUM_WAVES
                                    };

                                    opticalSystem.primWave = std::stoi(tokens[PRIM_WAVE]);

                                    int numWaves = std::stoi(tokens[NUM_WAVES]);
                                    if (numWaves < ZemaxDDE::MIN_WAVES || numWaves > ZemaxDDE::MAX_WAVES) {
                                        logger.addLog("[DDE] GetWave: Invalid numWaves value: " + std::to_string(numWaves) +
                                                    ". Must be in range [" + std::to_string(ZemaxDDE::MIN_WAVES) + ", " +
                                                    std::to_string(ZemaxDDE::MAX_WAVES) + "]");
                                        return 0;
                                    }

                                    opticalSystem.numWaves = numWaves;
                                } catch (const std::exception& e) {
                                    logger.addLog("[DDE] GetWave: Failed to parse wave metadata: " + std::string(e.what()));
                                    return 0;
                                }
                            } else {
                                const int GET_WAVE_DATA_COUNT = 2;     // waveLen, weight
                                if (dataCount != GET_WAVE_DATA_COUNT) {
                                    logger.addLog("[DDE] GetWave: Invalid parameter count for wave data. Expected exactly " +
                                                std::to_string(GET_WAVE_DATA_COUNT) + ", got " +
                                                std::to_string(dataCount));
                                    return 0;
                                }

                                try {
                                    enum {
                                        WAVE_LENGTH,
                                        WEIGHT,
                                    };

                                    opticalSystem.waveData[arg].value  = std::stod(tokens[WAVE_LENGTH]);
                                    opticalSystem.waveData[arg].weight = std::stod(tokens[WEIGHT]);
                                } catch (const std::exception& e) {
                                    logger.addLog("[DDE] GetWave: Failed to parse data for wave " + std::to_string(arg) +
                                                ": " + std::string(e.what()));
                                    return 0;
                                }
                            }
                        } else {
                            logger.addLog("[DDE] GetWave: Wave index must be 0 (metadata) or in range [" + 
                                        std::to_string(ZemaxDDE::MIN_WAVES) + ", " +
                                        std::to_string(ZemaxDDE::MAX_WAVES) + "]. Got: " +
                                        std::to_string(arg));
                            return 0;
                        }
                        DdeAck.fAck = TRUE;
                    }
                    if (command_token == "GetSurfaceData") {
                        auto tokens = ZemaxDDE::tokenize(buffer);
                        // int params = static_cast<int>(tokens.size());

                        int currentSurface = std::stoi(item_tokens[1]);
                        if (currentSurface < 0 || currentSurface > opticalSystem.numSurfs) {
                            logger.addLog("[DDE] GetSurfaceData: Invalid current surface value: " + std::to_string(currentSurface) +
                                        ". Must be in range [" + std::to_string(0) + ", " +
                                        std::to_string(opticalSystem.numSurfs) + "]");
                            return 0;
                        }

                        int code = std::stoi(item_tokens[2]);
                        if (!ZemaxDDE::SurfaceDataCode::isValid(code)) {
                            logger.addLog("[DDE] GetSurfaceData: Invalid surface data code received: " + std::to_string(code));
                            return 0;
                        }

                        // int arg2 = std::stoi(item_tokens[3]);

                        // Selecting target storage
                        auto& surface = (currentStorageTarget == StorageTarget::NOMINAL)
                            ? nominalSurface
                            : tolerancedSurface;
                        surface.id = currentSurface;

                        switch (code) {
                            case ZemaxDDE::SurfaceDataCode::TYPE_NAME:{
                                surface.type = tokens[0];
                                break;
                            }
                            case ZemaxDDE::SurfaceDataCode::SEMI_DIAMETER: {
                                surface.semiDiameter = std::stod(tokens[0]);
                                break;
                            }
                            default: {
                                logger.addLog("[DDE] GetSurfaceData: Unsupported code for storage: " + std::to_string(code));
                                return 0;
                            }
                        }

                        DdeAck.fAck = TRUE;
                        return 0;
                    }
                    if (command_token == "GetSag") {
                        auto tokens = ZemaxDDE::tokenize(buffer);
                        // int params = static_cast<int>(tokens.size());

                        int currentSurface = std::stoi(item_tokens[1]);
                        if (currentSurface < 0 || currentSurface > opticalSystem.numSurfs) {
                            logger.addLog("[DDE] GetSag: Invalid current surface value: " + std::to_string(currentSurface) +
                                        ". Must be in range [" + std::to_string(0) + ", " +
                                        std::to_string(opticalSystem.numSurfs) + "]");
                            return 0;
                        }

                        double x = 0.0;
                        double y = 0.0;

                        try {
                            x = std::stod(item_tokens[2]);
                            y = std::stod(item_tokens[3]);
                        } catch (const std::exception& e) {
                            logger.addLog("[DDE] GetSag: Failed to parse x/y: " + std::string(e.what()));
                            return 0;
                        }
                        
                        auto values = ZemaxDDE::tokenize(buffer);
                        if (values.empty()) {
                            logger.addLog("[DDE] GetSag: No values in response");
                            return 0;
                        }

                        double sag = std::stod(values[0]);
                        double alternateSag = std::stod(values[1]);

                        // Selecting target storage
                        auto& surface = (currentStorageTarget == StorageTarget::NOMINAL)
                            ? nominalSurface
                            : tolerancedSurface;
                        surface.id = currentSurface;
                        surface.sagDataPoints.push_back({
                            x, y, sag, alternateSag
                        });

                        DdeAck.fAck = TRUE;
                        return 0;    
                    }
                }
                if (pDdeData->fAckReq == TRUE) {
                    WORD wStatus;
                    memcpy(&wStatus, &DdeAck, sizeof(wStatus));
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
            logger.addLog("[DDE] " + std::string(DDE_ERROR_MSG_CONNECTION_NOT_ESTABLISHED));
        #endif
            throw std::runtime_error(DDE_ERROR_MSG_CONNECTION_NOT_ESTABLISHED);
        }
    }

    void ZemaxDDEClient::checkResponseStatus(const std::string& errorMsg) {
        if (!isDataReceived) {
        #ifdef DEBUG_LOG
            logger.addLog("[DDE] " + errorMsg);
        #endif
            throw std::runtime_error(errorMsg);
        }
    }
}
