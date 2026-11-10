#include <format>
#include <memory>
#include <utility>

#include "lens_update.h"

#include "client.h"
#include "logger/logger.h"

namespace ZemaxDDE {
    void requestLensRefresh(ZemaxDDEClient& client, Logger& logger,
                            std::function<void(UpdateStatus)> onDone,
                            DWORD timeoutMs, int retries, const std::string& serviceId) {
        // Both DDE callbacks must be able to fire, so the handler is shared
        // instead of being moved into one of them (a moved-from std::function
        // would drop the error path and leave the caller waiting forever).
        auto callback = std::make_shared<std::function<void(UpdateStatus)>>(std::move(onDone));

        logger.addLog("[DDE] GetRefresh: copying the LDE into the server lens");
        client.submitRequest("GetRefresh",
            [callback, &logger](const std::string& reply) {
                const UpdateStatus status = parseUpdateStatus(reply);
                if (status == UpdateStatus::NotUpdated) {
                    logger.addLog("[DDE] GetRefresh returned \"-1\": the lens cannot be updated");
                }
                if (*callback) {
                    (*callback)(status);
                }
            },
            [callback, &logger](const std::string& error) {
                logger.addLog(std::format("[DDE] GetRefresh failed: {}", error));
                if (*callback) {
                    (*callback)(UpdateStatus::Unknown);
                }
            },
            timeoutMs, retries, serviceId);
    }
}
