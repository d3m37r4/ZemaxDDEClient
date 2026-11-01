#pragma once

#include <string>

#include "lib/imgui/imgui.h"

class Logger;
namespace app::models { struct SurfaceData; }

namespace gui::plot_export {

    /// One plot capture request: the plot's draw list plus the resolution it
    /// must be rendered at. The draw list stays valid until the next
    /// ImGui::NewFrame(), so jobs are drained in the same frame from
    /// GraphicsBackend::endFrame().
    struct Job {
        ImDrawList* drawList = nullptr;
        ImVec2 displayPos{};
        ImVec2 displaySize{};
        int targetWidth = 0;
        int targetHeight = 0;
        std::string targetPath;
        Logger* logger = nullptr;
    };

    /// Queues a single capture; a newer job replaces an unprocessed older one
    /// (the toolbar cannot start two captures within one frame anyway).
    void queueJob(const Job& job);

    /// Renders the pending job into an offscreen FBO at the requested
    /// resolution, reads it back and writes it to job.targetPath. Must run
    /// after ImGui::Render() with the main GL context current.
    void renderPendingJob();

    /// Releases the offscreen target; call before the GL context is destroyed.
    void shutdown();

    /// Native save dialog (PNG / JPEG / BMP by extension). Returns false when
    /// the user cancelled; a dialog error is logged.
    bool chooseSavePath(Logger& logger, const std::string& defaultName, std::string& outPath);

    /// Full toolbar flow for one plot: asks for a path with the given export
    /// resolution, then queues the capture of the plot rect.
    void requestSave(Logger& logger, const char* kind, const app::models::SurfaceData& surface,
                     ImDrawList* drawList, const ImVec2& plotPos, const ImVec2& plotSize,
                     const ImVec2& resolution);

}
