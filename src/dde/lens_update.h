#pragma once

#include <functional>
#include <string>

#include "utils.h"

class Logger;

namespace ZemaxDDE {
    class ZemaxDDEClient;

    /// Default service id. Callers that must cancel only their own refresh pass
    /// a unique id and reuse it for cancelPendingRequests().
    inline constexpr const char* kLensRefreshServiceId = "LensRefresh";

    /// Sends "GetRefresh" and reports the outcome exactly once.
    /// Transport error, timeout, cancellation and an unrecognized reply all map
    /// to UpdateStatus::Unknown so the caller can decide whether to continue
    /// (see the GetRefresh plan: Unknown means "warn and proceed").
    void requestLensRefresh(ZemaxDDEClient& client, Logger& logger,
                            std::function<void(UpdateStatus)> onDone,
                            DWORD timeoutMs,
                            int retries = 1,
                            const std::string& serviceId = kLensRefreshServiceId);
}
