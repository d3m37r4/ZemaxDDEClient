#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <vector>

#include "client.h"
#include "initial_data_load_service.h"
#include "operation_monitor.h"
#include "utils.h"
#include "logger/logger.h"

namespace ZemaxDDE {
    ZemaxDDEClient::ZemaxDDEClient(HWND hwndClient, Logger& logger)
        : m_hwndZemaxClient(hwndClient)
        , m_logger(logger)
        , m_initialDataLoad(std::make_unique<InitialDataLoadService>(*this, m_opticalSystem, m_logger))
        , m_operationMonitor(std::make_unique<OperationMonitor>())
    {}

    ZemaxDDEClient::~ZemaxDDEClient() {
        terminateDDE();
    }

    void ZemaxDDEClient::setOnDDEConnectedCallback(OnDDEConnectedCallback callback) {
        m_onDDEConnected = callback;
    }

    void ZemaxDDEClient::initiateDDE() {
        if (m_hwndZemaxServer != nullptr) {
            m_logger.addLog("[DDE] DDE already connected. Skipping initiate.");
            return;
        }

        m_hwndZemaxServer = nullptr;

        ATOM appAtom = GlobalAddAtomW(DDE_APP_NAME);
        ATOM topicAtom = GlobalAddAtomW(DDE_TOPIC);
        DWORD_PTR dwResult = 0;
        if (!SendMessageTimeoutW(HWND_BROADCAST, WM_DDE_INITIATE, (WPARAM)m_hwndZemaxClient, MAKELONG(appAtom, topicAtom),
                SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, DDE_TIMEOUT_MS, &dwResult)) {
            m_logger.addLog("[DDE] WM_DDE_INITIATE broadcast timed out or failed (hung window detected)");
        }

        #ifdef DEBUG_LOG
        char appName[256], topicName[256];
        WideCharToMultiByte(CP_ACP, 0, DDE_APP_NAME, -1, appName, sizeof(appName), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, DDE_TOPIC, -1, topicName, sizeof(topicName), NULL, NULL);
        m_logger.addLog(std::format("[DDE] Sent 'WM_DDE_INITIATE' to app='{}', topic='{}'.", appName, topicName));
        #endif
    
        GlobalDeleteAtom(appAtom);
        GlobalDeleteAtom(topicAtom);
        checkDDEConnection();

        if (m_hwndZemaxServer) {
            m_logger.addLog("[DDE] Connection established successfully");
        } else {
            m_logger.addLog("[DDE] Connection not established yet (waiting for 'WM_DDE_ACK')");
        }

        if (m_onDDEConnected) {
            try {
                m_onDDEConnected(this);
            } catch (const std::exception& e) {
                m_logger.addLog(std::format("[DDE] Error in DDE connected callback: {}", e.what()));
                throw;
            }
        }
    }

    void ZemaxDDEClient::initiateDDE(HWND targetHwnd) {
        if (m_hwndZemaxServer != nullptr) {
            m_logger.addLog("[DDE] DDE already connected. Skipping initiate.");
            return;
        }

        m_hwndZemaxServer = nullptr;

        ATOM appAtom = GlobalAddAtomW(DDE_APP_NAME);
        ATOM topicAtom = GlobalAddAtomW(DDE_TOPIC);
        DWORD_PTR dwResult = 0;

        SendMessageTimeoutW(targetHwnd, WM_DDE_INITIATE, (WPARAM)m_hwndZemaxClient, MAKELONG(appAtom, topicAtom),
            SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, DDE_TIMEOUT_MS, &dwResult);

        #ifdef DEBUG_LOG
        char appName[256], topicName[256];
        WideCharToMultiByte(CP_ACP, 0, DDE_APP_NAME, -1, appName, sizeof(appName), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, DDE_TOPIC, -1, topicName, sizeof(topicName), NULL, NULL);
        m_logger.addLog(std::format("[DDE] Sent 'WM_DDE_INITIATE' to app='{}', topic='{}'.", appName, topicName));
        #endif

        GlobalDeleteAtom(appAtom);
        GlobalDeleteAtom(topicAtom);
        checkDDEConnection();

        if (m_hwndZemaxServer) {
            m_logger.addLog("[DDE] Connection established successfully");
        } else {
            m_logger.addLog("[DDE] Connection not established yet (waiting for 'WM_DDE_ACK')");
        }

        if (m_onDDEConnected) {
            try {
                m_onDDEConnected(this);
            } catch (const std::exception& e) {
                m_logger.addLog(std::format("[DDE] Error in DDE connected callback: {}", e.what()));
                throw;
            }
        }
    }

    void ZemaxDDEClient::pumpMessages() {
        MSG msg;
        while (PeekMessageW(&msg, m_hwndZemaxClient, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
            DispatchMessageW(&msg);
        }
    }

    uint64_t ZemaxDDEClient::submitRequest(const std::string& command,
        std::function<void(const std::string&)> onSuccess,
        std::function<void(const std::string&)> onError,
        DWORD timeoutMs,
        int retries,
        const std::string& serviceId) {

        uint64_t id = m_nextRequestId++;
        DdeRequest req;
        req.id = id;
        req.command = command;
        req.onSuccess = std::move(onSuccess);
        req.onError = std::move(onError);
        req.timeoutMs = timeoutMs;
        req.retriesLeft = retries;
        req.serviceId = serviceId;
        req.startTime = GetTickCount();
        m_requestQueue.push_back(std::move(req));

        m_logger.addLog(std::format("[DDE] Submitted request #{}: '{}' (svc={}, timeout={}ms, retries={})",
            id, command, serviceId, timeoutMs, retries));

        if (!m_activeRequest) {
            dispatchNext();
        }

        return id;
    }

    void ZemaxDDEClient::dispatchNext() {
        if (m_requestQueue.empty()) return;

        DdeRequest req = std::move(m_requestQueue.front());
        m_requestQueue.pop_front();

        if (!m_hwndZemaxServer) {
            if (req.onError) req.onError("Zemax is not connected");
            dispatchNext();
            return;
        }

        m_activeRequest = std::move(req);
        sendRequest(*m_activeRequest);
    }

    void ZemaxDDEClient::sendRequest(DdeRequest& req) {
        int wideCharCount = MultiByteToWideChar(CP_ACP, 0,
            req.command.data(), static_cast<int>(req.command.size()),
            nullptr, 0);

        std::vector<wchar_t> wItem(wideCharCount + 1);
        MultiByteToWideChar(CP_ACP, 0,
            req.command.data(), static_cast<int>(req.command.size()),
            wItem.data(), wideCharCount);
        wItem[wideCharCount] = L'\0';

        ATOM aItem = GlobalAddAtomW(wItem.data());
        PostMessageW(m_hwndZemaxServer, WM_DDE_REQUEST,
            reinterpret_cast<WPARAM>(m_hwndZemaxClient),
            PackDDElParam(WM_DDE_REQUEST, CF_TEXT, aItem));

        req.startTime = GetTickCount();

        m_logger.addLog(std::format("[DDE] Sent request #{}: '{}'", req.id, req.command));
    }

    void ZemaxDDEClient::finishRequest() {
        m_activeRequest.reset();
        dispatchNext();
    }

    void ZemaxDDEClient::processTimeouts() {
        if (!m_activeRequest) return;

        DWORD now = GetTickCount();
        auto& req = *m_activeRequest;

        if (now - req.startTime < req.timeoutMs) return;

        if (req.retriesLeft > 0) {
            req.retriesLeft--;
            req.timeoutMs = static_cast<DWORD>(static_cast<double>(req.timeoutMs) * 1.5);
            sendRequest(req);

            m_logger.addLog(std::format("[DDE] Retry #{} for request #{}: '{}' (timeout={}ms)",
                m_activeRequest->retriesLeft, req.id, req.command, req.timeoutMs));
        } else {
            m_logger.addLog(std::format("[DDE] Request #{} timed out: '{}'", req.id, req.command));
            if (req.onError) {
                req.onError("Timeout");
            }
            finishRequest();
        }
    }

    void ZemaxDDEClient::terminateDDE() {
        if (m_hwndZemaxServer) {
            PostMessageW(m_hwndZemaxServer, WM_DDE_TERMINATE, (WPARAM)m_hwndZemaxClient, 0L);
            m_hwndZemaxServer = nullptr;
            m_logger.addLog("[DDE] Connection terminated");
        }
    }

    class GlobalLockGuard {
        HGLOBAL m_handle;
        void* m_ptr;
    public:
        explicit GlobalLockGuard(HGLOBAL handle) : m_handle(handle), m_ptr(GlobalLock(handle)) {}
        ~GlobalLockGuard() { if (m_ptr) GlobalUnlock(m_handle); }
        GlobalLockGuard(const GlobalLockGuard&) = delete;
        GlobalLockGuard& operator=(const GlobalLockGuard&) = delete;

        bool isValid() const { return m_ptr != nullptr; }
        void* get() const { return m_ptr; }
        template<typename T> T* as() const { return static_cast<T*>(m_ptr); }
    };

    LRESULT ZemaxDDEClient::handleDDEMessages(UINT iMsg, WPARAM wParam, LPARAM lParam) {
        ATOM aItem;
        UINT_PTR lowWord, highWord;
        std::string buffer;

        #ifdef DEBUG_LOG
        m_logger.addLog(std::format("[DDE] Received message: {}", iMsg));
        #endif

        switch (iMsg) {
            case WM_DDE_ACK: {
                if (!m_hwndZemaxServer) {
                    UnpackDDElParam(WM_DDE_ACK, lParam, &lowWord, &highWord);
                    FreeDDElParam(WM_DDE_ACK, lParam);

                    m_hwndZemaxServer = reinterpret_cast<HWND>(wParam);

                    if (m_initialDataLoad) {
                        m_initialDataLoad->start();
                    }

                    GlobalDeleteAtom(static_cast<ATOM>(lowWord));
                    GlobalDeleteAtom(static_cast<ATOM>(highWord));

                    #ifdef DEBUG_LOG
                    m_logger.addLog(std::format("[DDE] Received 'WM_DDE_ACK', m_hwndZemaxServer = {}", reinterpret_cast<uintptr_t>(m_hwndZemaxServer)));
                    #endif
                }

                return 0;
            }
            case WM_DDE_DATA: {
                // ====== CLIENT-SIDE ACK STRUCTURE ======
                // `clientAck` is what WE (the client) send back to the server in WM_DDE_ACK.
                // It is NOT a server-side data structure.
                // Field semantics (from client's perspective):
                //   • clientAck.fAck       — "I successfully received and processed the data"
                //   • clientAck.fBusy      — "I am NOT busy" (always 0; we don't defer)
                //   • clientAck.fRelease   — "I do NOT request server to free anything" (always 0)
                //   • clientAck.fAckReq    — "I do NOT request ACK for my own ACK" (always 0)
                DDEACK clientAck{};

                UnpackDDElParam(WM_DDE_DATA, lParam, &lowWord, &highWord);
                FreeDDElParam(WM_DDE_DATA, lParam);

                // ====== SERVER-SIDE DATA STRUCTURE ======
                // `serverData` is the DDEDATA structure the SERVER (Zemax) sent us.
                // We never modify it — we only read its flags and copy its Value[].
                // Field semantics (from server's perspective):
                //   • serverData->fAckReq  — "Server REQUESTS us to send WM_DDE_ACK"
                //   • serverData->fRelease — "Server REQUESTS us to GlobalFree the handle after reading"
                //   • serverData->fAck     — "Server is acknowledging a previous request"
                //   • serverData->fBusy    — "Server is busy, will respond later"
                GLOBALHANDLE ddeDataHandle = reinterpret_cast<GLOBALHANDLE>(reinterpret_cast<uintptr_t>(lowWord));
                GlobalLockGuard serverDataLock(ddeDataHandle);
                aItem = static_cast<ATOM>(highWord);

                // ====== C-1 FIX: validate GlobalLock before dereferencing ======
                // Pre-existing bug: previously, ddeDataLock was validated only at the
                // start of the CF_TEXT block. If GlobalLock returned nullptr (invalid
                // handle, race with server freeing the handle, memory pressure), code
                // would dereference nullptr when reading fAckReq / fRelease below,
                // causing a process crash. Now we validate ONCE up front and cache
                // the pointer for safe subsequent access.
                if (!serverDataLock.isValid()) {
                    // Cannot read server's data — drop atom and exit.
                    // No ACK sent (server will eventually timeout).
                    GlobalDeleteAtom(aItem);
                    return 0;
                }
                auto* serverData = serverDataLock.as<::DDEDATA>();

                if (serverData->cfFormat == CF_TEXT) {
                    char item[512]; 
                    wchar_t wItem[512];
                    GlobalGetAtomNameW(aItem, wItem, sizeof(wItem));
                    WideCharToMultiByte(CP_ACP, 0, wItem, -1, item, sizeof(item), NULL, NULL);

                    std::string dde_item_str(item);
                    std::vector<std::string> item_tokens = ZemaxDDE::tokenize(dde_item_str);
                    std::string command_token = item_tokens.empty() ? "" : item_tokens[0];

                    buffer = reinterpret_cast<char*>(serverData->Value);

                    #ifdef DEBUG_LOG
                    m_logger.addLog(std::format("[DDE] Received 'WM_DDE_DATA', content = {}", buffer));
                    #endif

                    bool matched = false;
                    if (m_activeRequest && dde_item_str == m_activeRequest->command) {
                        m_logger.addLog(std::format("[DDE] Completed request #{}: '{}' (svc={})",
                            m_activeRequest->id, m_activeRequest->command, m_activeRequest->serviceId));
                        if (m_activeRequest->onSuccess) {
                            m_activeRequest->onSuccess(buffer);
                        }
                        finishRequest();
                        clientAck.fAck = true;
                        matched = true;
                    }

                    if (!matched) {
                        #ifdef DEBUG_LOG
                        m_logger.addLog(std::format("[DDE] No matching active request, using fallback for '{}'", command_token));
                        #endif

                        if (command_token == "GetName") {
                            std::string name = extractStringFromDDE(ddeDataHandle);
                            if (name.empty()) {
                                name = "unknown";
                                m_logger.addLog("[DDE] GetName: empty lens name, using default 'unknown'");
                            }
                            m_opticalSystem.lensName = name;
                            clientAck.fAck = true;
                        }
                        if (command_token == "GetFile") {
                            std::string fileNameStr = extractStringFromDDE(ddeDataHandle);
                            m_opticalSystem.fileName = fileNameStr;
                            clientAck.fAck = true;
                        }
                        if (command_token == "GetSystem") {
                            const int GET_SYSTEM_PARAMS_COUNT = 9;
                            auto systemParams = ZemaxDDE::tokenize(buffer);
                            int params = static_cast<int>(systemParams.size());

                            if (params != GET_SYSTEM_PARAMS_COUNT) {
                                m_logger.addLog(std::format("[DDE] GetSystem: Invalid parameter count. Expected exactly {}, got {}",
                                                        GET_SYSTEM_PARAMS_COUNT, params));
                                return 0;
                            }

                            try {
                                enum {
                                    NUM_SURFS,
                                    UNIT_CODE,
                                    STOP_SURF,
                                    NON_AXIAL_FLAG,
                                    RAY_AIMING_TYPE,
                                    ADJUST_INDEX,
                                    TEMP,
                                    PRESSURE,
                                    GLOBAL_REF_SURF
                                };

                                m_opticalSystem.numSurfs      = std::stoi(systemParams[NUM_SURFS]);
                                m_opticalSystem.units         = std::stoi(systemParams[UNIT_CODE]);
                                m_opticalSystem.stopSurf      = std::stoi(systemParams[STOP_SURF]);
                                m_opticalSystem.nonAxialFlag  = std::stoi(systemParams[NON_AXIAL_FLAG]);
                                m_opticalSystem.rayAimingType = std::stoi(systemParams[RAY_AIMING_TYPE]);
                                m_opticalSystem.adjustIndex   = std::stoi(systemParams[ADJUST_INDEX]);
                                m_opticalSystem.temp          = std::stod(systemParams[TEMP]);
                                m_opticalSystem.pressure      = std::stod(systemParams[PRESSURE]);
                                m_opticalSystem.globalRefSurf = std::stoi(systemParams[GLOBAL_REF_SURF]);

                                clientAck.fAck = true;
                            } catch (const std::invalid_argument& e) {
                                m_logger.addLog(std::format("[DDE] GetSystem: Invalid number format in parameter: {}", e.what()));
                                return 0;
                            } catch (const std::out_of_range& e) {
                                m_logger.addLog(std::format("[DDE] GetSystem: Number out of range: {}", e.what()));
                                return 0;
                            } catch (const std::exception& e) {
                                m_logger.addLog(std::format("[DDE] GetSystem: Unexpected error: {}", e.what()));
                                return 0;
                            }
                        }
                        if (command_token == "GetField") {
                            const int EXPECTED_COMMAND_TOKENS = 2;
                            int params = static_cast<int>(item_tokens.size());

                            if (params != EXPECTED_COMMAND_TOKENS) {
                                m_logger.addLog(std::format("[DDE] GetField: Invalid command format. Expected exactly {} tokens, got {}",
                                                        EXPECTED_COMMAND_TOKENS, params));
                                return 0;
                            }

                            int arg = -1;
                            try {
                                arg = std::stoi(item_tokens[1]);
                            } catch (const std::invalid_argument&) {
                                m_logger.addLog(std::format("[DDE] GetField: Invalid field index '{}'. Not a number.", item_tokens[1]));
                                return 0;
                            } catch (const std::out_of_range&) {
                                m_logger.addLog(std::format("[DDE] GetField: Field index '{}' is out of range (too large).", item_tokens[1]));
                                return 0;
                            }

                            if (arg == 0 || (arg >= ZemaxDDE::MIN_FIELDS && arg <= ZemaxDDE::MAX_FIELDS)) {
                                auto tokens = ZemaxDDE::tokenize(buffer);
                                int dataCount = static_cast<int>(tokens.size());

                                if(arg == 0) {
                                    const int GET_FIELD_META_COUNT = 5;
                                    if (dataCount != GET_FIELD_META_COUNT) {
                                        m_logger.addLog(std::format("[DDE] GetField: Invalid parameter count for field metadata. Expected exactly {}, got {}",
                                                                GET_FIELD_META_COUNT, dataCount));
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

                                        m_opticalSystem.fieldType = std::stoi(tokens[FIELD_TYPE]);

                                        int numFields = std::stoi(tokens[NUM_FIELDS]);
                                        if (numFields < ZemaxDDE::MIN_FIELDS || numFields > ZemaxDDE::MAX_FIELDS) {
                                            m_logger.addLog(std::format("[DDE] GetField: Invalid numFields value: {}. Must be in range [{}, {}]",
                                                                    numFields, ZemaxDDE::MIN_FIELDS, ZemaxDDE::MAX_FIELDS));
                                            return 0;
                                        }

                                        m_opticalSystem.numFields = numFields;
                                        m_opticalSystem.maxXField = std::stod(tokens[MAX_X_FIELD]);
                                        m_opticalSystem.maxYField = std::stod(tokens[MAX_Y_FIELD]);
                                        m_opticalSystem.normalizationMethod = std::stoi(tokens[NORMALIZATION_METHOD]);
                                    } catch (const std::exception& e) {
                                        m_logger.addLog(std::format("[DDE] GetField: Failed to parse field metadata: {}", e.what()));
                                        return 0;
                                    }
                                } else {
                                    const int GET_FIELD_DATA_COUNT = 8;
                                    if (dataCount != GET_FIELD_DATA_COUNT) {
                                        m_logger.addLog(std::format("[DDE] GetField: Invalid parameter count for field data. Expected exactly {}, got {}",
                                                                GET_FIELD_DATA_COUNT, dataCount));
                                        return 0;
                                    }

                                    try {
                                        enum {
                                            XFIELD,
                                            YFIELD,
                                        };

                                        m_opticalSystem.xField[arg] = std::stod(tokens[XFIELD]);
                                        m_opticalSystem.yField[arg] = std::stod(tokens[YFIELD]);
                                    } catch (const std::exception& e) {
                                        m_logger.addLog(std::format("[DDE] GetField: Failed to parse data for field {}: {}", arg, e.what()));
                                        return 0;
                                    }
                                }
                            } else {
                                m_logger.addLog(std::format("[DDE] GetField: Field index must be 0 (metadata) or in range [{}, {}]. Got: {}",
                                                        ZemaxDDE::MIN_FIELDS, ZemaxDDE::MAX_FIELDS, arg));
                                return 0;
                            }
                            clientAck.fAck = true;
                        }
                        if (command_token == "GetWave") {
                            const int EXPECTED_COMMAND_TOKENS = 2;
                            int params = static_cast<int>(item_tokens.size());

                            if (params != EXPECTED_COMMAND_TOKENS) {
                                m_logger.addLog(std::format("[DDE] GetWave: Invalid command format. Expected exactly {} tokens, got {}",
                                                        EXPECTED_COMMAND_TOKENS, params));
                                return 0;
                            }

                            int arg = -1;
                            try {
                                arg = std::stoi(item_tokens[1]);
                            } catch (const std::invalid_argument&) {
                                m_logger.addLog(std::format("[DDE] GetWave: Invalid wave index '{}'. Not a number.", item_tokens[1]));
                                return 0;
                            } catch (const std::out_of_range&) {
                                m_logger.addLog(std::format("[DDE] GetWave: Wave index '{}' is out of range (too large).", item_tokens[1]));
                                return 0;
                            }

                            if (arg == 0 || (arg >= ZemaxDDE::MIN_WAVES && arg <= ZemaxDDE::MAX_WAVES)) {
                                auto tokens = ZemaxDDE::tokenize(buffer);
                                int dataCount = static_cast<int>(tokens.size());

                                if(arg == 0) {
                                    const int GET_WAVE_META_COUNT = 2;
                                    if (dataCount != GET_WAVE_META_COUNT) {
                                        m_logger.addLog(std::format("[DDE] GetWave: Invalid parameter count for wave metadata. Expected exactly {}, got {}",
                                                                GET_WAVE_META_COUNT, dataCount));
                                        return 0;
                                    }

                                    try {
                                        enum  {
                                            PRIM_WAVE,
                                            NUM_WAVES
                                        };

                                        m_opticalSystem.primWave = std::stoi(tokens[PRIM_WAVE]);

                                        int numWaves = std::stoi(tokens[NUM_WAVES]);
                                        if (numWaves < ZemaxDDE::MIN_WAVES || numWaves > ZemaxDDE::MAX_WAVES) {
                                            m_logger.addLog(std::format("[DDE] GetWave: Invalid numWaves value: {}. Must be in range [{}, {}]",
                                                                    numWaves, ZemaxDDE::MIN_WAVES, ZemaxDDE::MAX_WAVES));
                                            return 0;
                                        }

                                        m_opticalSystem.numWaves = numWaves;
                                    } catch (const std::exception& e) {
                                        m_logger.addLog(std::format("[DDE] GetWave: Failed to parse wave metadata: {}", e.what()));
                                        return 0;
                                    }
                                } else {
                                    const int GET_WAVE_DATA_COUNT = 2;
                                    if (dataCount != GET_WAVE_DATA_COUNT) {
                                        m_logger.addLog(std::format("[DDE] GetWave: Invalid parameter count for wave data. Expected exactly {}, got {}",
                                                                GET_WAVE_DATA_COUNT, dataCount));
                                        return 0;
                                    }

                                    try {
                                        enum {
                                            WAVE_LENGTH,
                                            WEIGHT,
                                        };

                                        m_opticalSystem.waveData[arg].value  = std::stod(tokens[WAVE_LENGTH]);
                                        m_opticalSystem.waveData[arg].weight = std::stod(tokens[WEIGHT]);
                                    } catch (const std::exception& e) {
                                        m_logger.addLog(std::format("[DDE] GetWave: Failed to parse data for wave {}: {}", arg, e.what()));
                                        return 0;
                                    }
                                }
                            } else {
                                m_logger.addLog(std::format("[DDE] GetWave: Wave index must be 0 (metadata) or in range [{}, {}]. Got: {}",
                                                        ZemaxDDE::MIN_WAVES, ZemaxDDE::MAX_FIELDS, arg));
                                return 0;
                            }
                            clientAck.fAck = true;
                        }
                        if (command_token == "GetSurfaceData") {
                            clientAck.fAck = true;
                        }
                        if (command_token == "GetSag") {
                            clientAck.fAck = true;
                        }
                    }
                }
                if (serverData->fAckReq == true) {
                    // Server wants our ACK. Build the status word per DDE protocol:
                    //   bit 13: fAck      (clientAck.fAck  ? 1 : 0)
                    //   bit 12: fBusy     (clientAck.fBusy ? 1 : 0 — always 0; client never busy)
                    //   bits 0-11, 14-15: unused/reserved (0)
                    //
                    // NOTE: DDEACK in MinGW's <dde.h> only has fields {unused, fBusy, fAck}.
                    // fRelease is a SERVER-side flag (in DDEDATA), not part of the client's
                    // outgoing DDEACK, so it is correctly omitted here. Constructed
                    // explicitly (not via memcpy of the DDEACK struct) so the bit
                    // positions match the wire protocol regardless of how the compiler
                    // arranges the bitfields in DDEACK (MSVC vs MinGW/GCC differ).
                    static_assert(sizeof(::DDEACK) == sizeof(WORD),
                        "DDEACK must be 2 bytes; PackDDElParam requires WORD for WM_DDE_ACK");
                    WORD wStatus = static_cast<WORD>(
                        (clientAck.fAck  ? 0x2000 : 0) |
                        (clientAck.fBusy ? 0x1000 : 0)
                    );
                    if (!PostMessageW(reinterpret_cast<HWND>(wParam), WM_DDE_ACK, reinterpret_cast<WPARAM>(m_hwndZemaxClient), PackDDElParam(WM_DDE_ACK, wStatus, aItem))) {
                        GlobalDeleteAtom(aItem);
                        GlobalFree(ddeDataHandle);
                        return 0;
                    }
                } else {
                    GlobalDeleteAtom(aItem);
                }

                // Free the handle per DDE protocol in two cases:
                //   1) Server explicitly requested release (serverData->fRelease = true).
                //      This is the standard happy path for large data.
                //   2) We did NOT acknowledge success (clientAck.fAck = false).
                //      Cleanup on error to prevent handle leak. Per DDE protocol,
                //      if the client cannot process the data, it should still free
                //      the handle to avoid memory leak in the server's process.
                if (serverData->fRelease == true || clientAck.fAck == false) {
                    GlobalFree(ddeDataHandle);
                }
                return 0;
            }
        }
        return 0;
    }

    void ZemaxDDEClient::checkDDEConnection() {
        const char* const DDE_ERROR_MSG_CONNECTION_NOT_ESTABLISHED = "No ZemaxDDEServer received, DDE connection to Zemax not established";
        if (!m_hwndZemaxServer) {
            #ifdef DEBUG_LOG
            m_logger.addLog(std::format("[DDE] {}", DDE_ERROR_MSG_CONNECTION_NOT_ESTABLISHED));
            #endif

            throw std::runtime_error(DDE_ERROR_MSG_CONNECTION_NOT_ESTABLISHED);
        }
    }
}
