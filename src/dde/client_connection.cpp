#include <iterator>
#include <stdexcept>
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

    void ZemaxDDEClient::setConnectionState(ConnectionState newState) {
        if (m_connectionState != newState) {
            m_logger.addLog(std::format("[DDE] State: {} -> {}",
                toString(m_connectionState), toString(newState)));
            m_connectionState = newState;
        }
    }

    void ZemaxDDEClient::initiateDDE(HWND targetHwnd) {
        if (m_hwndZemaxServer != nullptr) {
            m_logger.addLog("[DDE] DDE already connected. Skipping initiate.");
            return;
        }

        setConnectionState(ConnectionState::Connecting);
        m_isConnecting = true;
        m_hwndZemaxServer = nullptr;

        ATOM appAtom = GlobalAddAtomW(DDE_APP_NAME);
        ATOM topicAtom = GlobalAddAtomW(DDE_TOPIC);
        DWORD_PTR dwResult = 0;

        SendMessageTimeoutW(targetHwnd, WM_DDE_INITIATE, (WPARAM)m_hwndZemaxClient, MAKELONG(appAtom, topicAtom),
            SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, m_defaultTimeoutMs, &dwResult);

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
            m_isConnecting = false;
            setConnectionState(ConnectionState::Connected);
            m_logger.addLog("[DDE] Connection established successfully");
        } else {
            m_logger.addLog("[DDE] Connection not established yet (waiting for 'WM_DDE_ACK')");
        }
    }

    uint64_t ZemaxDDEClient::submitRequest(const std::string& command,
        std::function<void(const std::string&)> onSuccess,
        std::function<void(const std::string&)> onError,
        DWORD timeoutMs,
        int retries,
        const std::string& serviceId) {

        // Sentinel: 0 / -1 → use client defaults driven by AppSettings.dde.
        if (timeoutMs == 0) timeoutMs = m_defaultTimeoutMs;
        if (retries   <  0) retries   = m_defaultRetries;

        uint64_t id = m_nextRequestId++;
        DdeRequest req;
        req.id = id;
        req.command = command;
        req.onSuccess = std::move(onSuccess);
        req.onError = std::move(onError);
        req.timeoutMs = timeoutMs;
        req.retriesLeft = retries;
        req.serviceId = serviceId;
        m_requestQueue.push_back(std::move(req));

        m_logger.addLog(std::format("[DDE] Submitted request #{}: '{}' (svc={}, timeout={}ms, retries={})",
            id, command, serviceId, timeoutMs, retries));

        if (!m_activeRequest) {
            dispatchNext();
        }

        return id;
    }

    void ZemaxDDEClient::cancelPendingRequests(const std::string& serviceId) {
        std::vector<std::function<void(const std::string&)>> failures;

        // Rebuild the queue without the cancelled service's entries; the error
        // callbacks are collected and invoked afterwards so a re-entrant
        // submitRequest (from within a callback) cannot corrupt the iteration.
        std::deque<DdeRequest> kept;
        for (auto& req : m_requestQueue) {
            if (req.serviceId == serviceId) {
                m_logger.addLog(std::format("[DDE] Request #{} cancelled before dispatch", req.id));
                if (req.onError) failures.push_back(std::move(req.onError));
            } else {
                kept.push_back(std::move(req));
            }
        }
        m_requestQueue = std::move(kept);

        for (auto& onError : failures) {
            onError("Cancelled");
        }
    }

    void ZemaxDDEClient::dispatchNext() {
        // Defensive: caller contract is that m_activeRequest is null here.
        // If not, do nothing to avoid overwriting an in-flight request.
        // Public callers (submitRequest) already check this, but the guard
        // makes the function idempotent for any future internal callers.
        if (m_activeRequest) return;

        // Track consecutive 'Zemax is not connected' errors in this call to
        // emit a diagnostic warning when a server-side outage causes a batch
        // of failures (avoids per-request log spam for normal cases).
        size_t consecutiveErrors = 0;

        // Iterative loop: avoid unbounded recursion if m_hwndZemaxServer is
        // null and the queue holds many failed requests
        // Each iteration processes one request; the loop drains the queue
        // until either an active request is dispatched (return) or the
        // queue is empty.
        while (!m_requestQueue.empty()) {
            DdeRequest req = std::move(m_requestQueue.front());
            m_requestQueue.pop_front();

            if (m_connectionState != ConnectionState::Connected) {
                if (req.onError) req.onError("Zemax is not connected");
                ++consecutiveErrors;
                continue;
            }

            m_activeRequest = std::move(req);
            if (!sendRequest(*m_activeRequest)) {
                // Drop the request and keep draining in this loop: recursing
                // through the old finishRequest() grew the stack by one frame
                // per failed send. The owner still receives a failure callback.
                DdeRequest failed = takeActiveRequest();
                if (failed.onError) {
                    try {
                        failed.onError("Failed to submit request (PostMessage failed)");
                    } catch (const std::exception& e) {
                        m_logger.addLog(std::format("[DDE] CRITICAL: Exception in onError: {}", e.what()));
                    } catch (...) {
                        m_logger.addLog("[DDE] CRITICAL: Unknown exception in onError");
                    }
                }
                continue;
            }

            if (consecutiveErrors >= kMassErrorWarnThreshold) {
                m_logger.addLog(std::format(
                    "[DDE] Warning: {} consecutive requests failed with "
                    "'Zemax is not connected' before a successful dispatch. "
                    "Server may have disconnected mid-batch.",
                    consecutiveErrors));
            }
            return;  // active request set; wait for the response or timeout
        }

        // Queue drained without setting an active request (all errored).
        if (consecutiveErrors >= kMassErrorWarnThreshold) {
            m_logger.addLog(std::format(
                "[DDE] Warning: {} consecutive requests failed with "
                "'Zemax is not connected'. Queue is empty; server is likely "
                "disconnected.",
                consecutiveErrors));
        }
    }

    bool ZemaxDDEClient::sendRequest(DdeRequest& req) {
        int wideCharCount = MultiByteToWideChar(CP_ACP, 0,
            req.command.data(), static_cast<int>(req.command.size()),
            nullptr, 0);

        std::vector<wchar_t> wItem(wideCharCount + 1);
        MultiByteToWideChar(CP_ACP, 0,
            req.command.data(), static_cast<int>(req.command.size()),
            wItem.data(), wideCharCount);
        wItem[wideCharCount] = L'\0';

        ATOM aItem = GlobalAddAtomW(wItem.data());
        req.itemAtom = aItem;

        if (!PostMessageW(m_hwndZemaxServer, WM_DDE_REQUEST,
                reinterpret_cast<WPARAM>(m_hwndZemaxClient),
                PackDDElParam(WM_DDE_REQUEST, CF_TEXT, aItem))) {
            m_logger.addLog(std::format("[DDE] PostMessageW failed for request #{}: '{}'", req.id, req.command));
            GlobalDeleteAtom(aItem);
            req.itemAtom = 0;
            return false;
        }

        req.startTime = std::chrono::steady_clock::now();
        m_logger.addLog(std::format("[DDE] Sent request #{}: '{}'", req.id, req.command));
        return true;
    }

    DdeRequest ZemaxDDEClient::takeActiveRequest() {
        if (!m_activeRequest) return {};
        if (m_activeRequest->itemAtom != 0) {
            GlobalDeleteAtom(m_activeRequest->itemAtom);
            m_activeRequest->itemAtom = 0;
        }
        DdeRequest request = std::move(*m_activeRequest);
        m_activeRequest.reset();
        return request;
    }

    void ZemaxDDEClient::processTimeouts() {
        if (!m_activeRequest) return;

        auto now = std::chrono::steady_clock::now();
        auto& req = *m_activeRequest;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - req.startTime);
        if (elapsed.count() < req.timeoutMs) return;

        bool resendFailed = false;
        if (req.retriesLeft > 0) {
            if (req.itemAtom != 0) {
                GlobalDeleteAtom(req.itemAtom);
                req.itemAtom = 0;
            }
            req.retriesLeft--;
            req.timeoutMs = static_cast<DWORD>(static_cast<double>(req.timeoutMs) * 1.5);
            if (sendRequest(req)) {
                m_logger.addLog(std::format("[DDE] Retry #{} for request #{}: '{}' (timeout={}ms)",
                    req.retriesLeft, req.id, req.command, req.timeoutMs));
                return;
            }
            // Re-send failed (e.g. PostMessageW error). The atom was already
            // released inside sendRequest; treat the retry as a hard failure
            // instead of leaving the request suspended in limbo.
            resendFailed = true;
            m_logger.addLog(std::format("[DDE] ERROR: Retry resend failed for request #{}: '{}'",
                req.id, req.command));
        } else {
            m_logger.addLog(std::format("[DDE] ERROR: Request #{} timed out (max retries exceeded)",
                req.id));
        }

        // Release the request before its callback runs: the callback may cancel
        // the owner or submit the next request and must never observe this one
        // as still in flight.
        DdeRequest completed = takeActiveRequest();
        if (completed.onError) {
            try {
                completed.onError(resendFailed
                    ? "Failed to resend request (PostMessage failed)"
                    : "Timeout: max retries exceeded");
            } catch (const std::exception& e) {
                m_logger.addLog(std::format("[DDE] CRITICAL: Exception in onError: {}", e.what()));
            } catch (...) {
                m_logger.addLog("[DDE] CRITICAL: Unknown exception in onError");
            }
        }
        dispatchNext();
    }

    void ZemaxDDEClient::checkConnectionHealth() {
        if (m_connectionState != ConnectionState::Connected) return;

        if (!IsWindow(m_hwndZemaxServer)) {
            handleConnectionLost("Zemax window handle is no longer valid");
            return;
        }

        DWORD currentPid = 0;
        GetWindowThreadProcessId(m_hwndZemaxServer, &currentPid);
        if (m_serverPid != 0 && currentPid != m_serverPid) {
            handleConnectionLost("Zemax process ID changed (process restarted?)");
            return;
        }

        if (currentPid != 0) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, currentPid);
            if (hProcess) {
                DWORD exitCode;
                if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                    handleConnectionLost("Zemax process has exited");
                    CloseHandle(hProcess);
                    return;
                }
                CloseHandle(hProcess);
            }
        }
    }

    void ZemaxDDEClient::handleConnectionLost(const std::string& reason) {
        m_logger.addLog(std::format("[DDE] ERROR: Connection lost — {}", reason));
        m_isConnecting = false;
        setConnectionState(ConnectionState::Disconnected);
        m_hwndZemaxServer = nullptr;
        drainRequestQueue(reason);
        m_connectionLost = true;
        m_connectionLostReason = reason;
    }

    void ZemaxDDEClient::drainRequestQueue(const std::string& reason) {
        if (m_activeRequest) {
            // Release the request BEFORE its callback: the old finishRequest()
            // dispatched the next request mid-drain, which then failed with
            // "Zemax is not connected" instead of this connection-lost reason.
            DdeRequest failed = takeActiveRequest();
            if (failed.onError) {
                try {
                    failed.onError("Connection lost: " + reason);
                } catch (const std::exception& e) {
                    m_logger.addLog(std::format("[DDE] CRITICAL: Exception in onError: {}", e.what()));
                } catch (...) {
                    m_logger.addLog("[DDE] CRITICAL: Unknown exception in onError");
                }
            }
        }

        while (!m_requestQueue.empty()) {
            auto req = std::move(m_requestQueue.front());
            m_requestQueue.pop_front();

            if (req.itemAtom != 0) {
                GlobalDeleteAtom(req.itemAtom);
                req.itemAtom = 0;
            }

            if (req.onError) {
                try {
                    req.onError("Connection lost: " + reason);
                } catch (const std::exception& e) {
                    m_logger.addLog(std::format("[DDE] CRITICAL: Exception in onError: {}", e.what()));
                } catch (...) {
                    m_logger.addLog("[DDE] CRITICAL: Unknown exception in onError");
                }
            }
        }
    }

    void ZemaxDDEClient::terminateDDE() {
        if (m_hwndZemaxServer) {
            PostMessageW(m_hwndZemaxServer, WM_DDE_TERMINATE, (WPARAM)m_hwndZemaxClient, 0L);
            m_logger.addLog("[DDE] Connection terminated");
        }
        m_isConnecting = false;
        setConnectionState(ConnectionState::Disconnected);
        m_hwndZemaxServer = nullptr;
        drainRequestQueue("Manual disconnect");
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
        /// Releases the lock explicitly. Required before GlobalFree(): freeing a
        /// still-locked block and unlocking the freed handle afterwards is
        /// undefined behaviour (and corrupts the global heap).
        void unlock() noexcept {
            if (m_ptr) {
                GlobalUnlock(m_handle);
                m_ptr = nullptr;
            }
        }
    };

    LRESULT ZemaxDDEClient::handleDDEMessages(UINT iMsg, WPARAM wParam, LPARAM lParam) {
        UINT_PTR lowWord, highWord;

        #ifdef DEBUG_LOG
        m_logger.addLog(std::format("[DDE] Received message: {}", iMsg));
        #endif

        switch (iMsg) {
            case WM_DDE_ACK: {
                if (!m_hwndZemaxServer && m_isConnecting) {
                    UnpackDDElParam(WM_DDE_ACK, lParam, &lowWord, &highWord);
                    FreeDDElParam(WM_DDE_ACK, lParam);

                    m_hwndZemaxServer = reinterpret_cast<HWND>(wParam);
                    m_isConnecting = false;
                    setConnectionState(ConnectionState::Connected);

                    DWORD pid = 0;
                    GetWindowThreadProcessId(m_hwndZemaxServer, &pid);
                    m_serverPid = pid;

                    if (m_initialDataLoad) {
                        m_initialDataLoad->start();
                    }

                    GlobalDeleteAtom(static_cast<ATOM>(lowWord));
                    GlobalDeleteAtom(static_cast<ATOM>(highWord));

                    #ifdef DEBUG_LOG
                    m_logger.addLog(std::format("[DDE] Received 'WM_DDE_ACK', m_hwndZemaxServer = {}", reinterpret_cast<uintptr_t>(m_hwndZemaxServer)));
                    #endif
                } else {
                    UINT_PTR wStatus = 0;
                    UINT_PTR aItemAck = 0;
                    UnpackDDElParam(WM_DDE_ACK, lParam, &wStatus, &aItemAck);
                    FreeDDElParam(WM_DDE_ACK, lParam);

                    if (m_activeRequest) {
                        // NOTE (verify): the DDEACK structure defines fAck as bit 15
                        // (0x8000) and fBusy as bit 14 (0x4000). Here the code tests
                        // 0x2000/0x1000 instead, which deviates from the DDE spec but
                        // appears to match what Zemax's server actually sends. Any
                        // change requires verification against a real Zemax instance;
                        // only the semantics (ack/busy flags) are consumed below.
                        bool isAck = (wStatus & 0x2000) != 0;
                        bool isBusy = (wStatus & 0x1000) != 0;

                        if (!isAck && !isBusy) {
                            m_logger.addLog(std::format("[DDE] Request #{} rejected by server (Negative ACK)", m_activeRequest->id));
                            DdeRequest rejected = takeActiveRequest();
                            if (rejected.onError) {
                                try {
                                    rejected.onError("Server rejected the request (Negative ACK)");
                                } catch (const std::exception& e) {
                                    m_logger.addLog(std::format("[DDE] CRITICAL: Exception in onError: {}", e.what()));
                                } catch (...) {
                                    m_logger.addLog("[DDE] CRITICAL: Unknown exception in onError");
                                }
                            }
                            dispatchNext();
                        }
                    }

                    // The ACK item atom is owned by us after UnpackDDElParam; release it
                    // in all sub-cases (positive ack, busy, negative ack, stale) to avoid
                    // a per-message atom leak.
                    if (aItemAck != 0) {
                        GlobalDeleteAtom(static_cast<ATOM>(aItemAck));
                    }
                }

                return 0;
            }
            case WM_DDE_TERMINATE: {
                // DDE requires the client to answer the server's terminate so the
                // conversation closes on both sides.
                PostMessageW(reinterpret_cast<HWND>(wParam), WM_DDE_TERMINATE,
                             reinterpret_cast<WPARAM>(m_hwndZemaxClient), 0L);
                m_logger.addLog("[DDE] Received WM_DDE_TERMINATE from server");
                handleConnectionLost("Server sent WM_DDE_TERMINATE");
                return 0;
            }
            case WM_DDE_DATA: {
                UnpackDDElParam(WM_DDE_DATA, lParam, &lowWord, &highWord);
                FreeDDElParam(WM_DDE_DATA, lParam);

                GLOBALHANDLE ddeDataHandle =
                    reinterpret_cast<GLOBALHANDLE>(reinterpret_cast<uintptr_t>(lowWord));
                ATOM aItem = static_cast<ATOM>(highWord);

                GlobalLockGuard serverDataLock(ddeDataHandle);
                if (!serverDataLock.isValid()) {
                    m_logger.addLog("[DDE] ERROR: Invalid GlobalLock in WM_DDE_DATA");
                    // Broken packet: release what we own and leave the active
                    // request on its retry/timeout path instead of finishing it.
                    GlobalDeleteAtom(aItem);
                    if (ddeDataHandle) {
                        GlobalFree(ddeDataHandle);
                    }
                    return 0;
                }

                auto* serverData = serverDataLock.as<::DDEDATA>();
                // DDEDATA flags are read before the block can be released.
                const bool ackRequested = serverData->fAckReq != 0;
                const bool releaseData = serverData->fRelease != 0;
                const bool textFormat = serverData->cfFormat == CF_TEXT;

                bool requestFulfilled = false;
                std::string payload;
                if (m_activeRequest && textFormat) {
                    char item[512];
                    wchar_t wItem[512];
                    GlobalGetAtomNameW(aItem, wItem, std::size(wItem));
                    WideCharToMultiByte(CP_ACP, 0, wItem, -1, item, sizeof(item), NULL, NULL);

                    if (std::string(item) == m_activeRequest->command) {
                        requestFulfilled = true;

                        #ifdef DEBUG_LOG
                        m_logger.addLog(std::format("[DDE] Received data for #{}: '{}'",
                            m_activeRequest->id, m_activeRequest->command));
                        #endif

                        // Build a length-bounded payload from the global memory block so
                        // that non-null-terminated / binary data cannot overread past the
                        // allocated buffer (Value is a flexible trailing array).
                        static constexpr size_t kHeaderSize = offsetof(::DDEDATA, Value);
                        const size_t blockSize = GlobalSize(ddeDataHandle);
                        if (blockSize > kHeaderSize) {
                            payload.assign(reinterpret_cast<const char*>(serverData->Value),
                                           blockSize - kHeaderSize);
                        }
                    }
                }

                // Atom ownership: an ACK hands the item back to the server, so the
                // client must not delete it when the post succeeds. Without an ACK
                // request (or when the post fails) the atom stays ours to release.
                if (ackRequested) {
                    static_assert(sizeof(::DDEACK) == sizeof(WORD),
                        "DDEACK must be 2 bytes; PackDDElParam requires WORD for WM_DDE_ACK");
                    DDEACK clientAck{};
                    clientAck.fAck = requestFulfilled ? TRUE : FALSE;
                    const WORD wStatus = static_cast<WORD>(
                        (clientAck.fAck  ? 0x2000 : 0) |
                        (clientAck.fBusy ? 0x1000 : 0)
                    );
                    const LPARAM ackParam = PackDDElParam(WM_DDE_ACK, wStatus, aItem);
                    if (!PostMessageW(reinterpret_cast<HWND>(wParam), WM_DDE_ACK,
                                      reinterpret_cast<WPARAM>(m_hwndZemaxClient), ackParam)) {
                        FreeDDElParam(WM_DDE_ACK, ackParam);
                        GlobalDeleteAtom(aItem);
                    }
                } else {
                    GlobalDeleteAtom(aItem);
                }

                // Never free a locked block: unlock first, otherwise the guard's
                // destructor unlocks the freed handle (global heap corruption).
                if (releaseData) {
                    serverDataLock.unlock();
                    GlobalFree(ddeDataHandle);
                }

                if (requestFulfilled) {
                    // Release the request before the callback: the owner may
                    // submit the next section or cancel from inside onSuccess.
                    DdeRequest completed = takeActiveRequest();
                    if (completed.onSuccess) {
                        try {
                            completed.onSuccess(payload);
                        } catch (const std::exception& e) {
                            m_logger.addLog(std::format("[DDE] CRITICAL: Exception in onSuccess: {}", e.what()));
                        } catch (...) {
                            m_logger.addLog("[DDE] CRITICAL: Unknown exception in onSuccess");
                        }
                    }
                    dispatchNext();
                } else {
                    m_logger.addLog(std::format("[DDE] WARNING: Data mismatch or no handler for atom {}", aItem));
                }
                return 0;
            }
        }

        // DDE messages we do not consume still hand us packed parameters: free
        // the param block and the item atom, otherwise both leak per message.
        switch (iMsg) {
            case WM_DDE_REQUEST:
            case WM_DDE_ADVISE:
            case WM_DDE_UNADVISE: {
                UINT_PTR format = 0;
                UINT_PTR item = 0;
                UnpackDDElParam(iMsg, lParam, &format, &item);
                FreeDDElParam(iMsg, lParam);
                if (item != 0) {
                    GlobalDeleteAtom(static_cast<ATOM>(item));
                }
                m_logger.addLog(std::format("[DDE] Dropped unsupported message {} (format={})", iMsg, format));
                return 0;
            }
            default:
                break;
        }
        return DefWindowProcW(m_hwndZemaxClient, iMsg, wParam, lParam);
    }

    void ZemaxDDEClient::checkDDEConnection() {
        const char* const DDE_ERROR_MSG_CONNECTION_NOT_ESTABLISHED = "No ZemaxDDEServer received, DDE connection to Zemax not established";
        if (m_connectionState != ConnectionState::Connected) {
            #ifdef DEBUG_LOG
            m_logger.addLog(std::format("[DDE] {}", DDE_ERROR_MSG_CONNECTION_NOT_ESTABLISHED));
            #endif

            throw std::runtime_error(DDE_ERROR_MSG_CONNECTION_NOT_ESTABLISHED);
        }
    }
}
