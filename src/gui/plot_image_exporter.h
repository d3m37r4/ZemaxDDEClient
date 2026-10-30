#pragma once

#include <string>

#include "lib/imgui/imgui.h"

class Logger;

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

}
