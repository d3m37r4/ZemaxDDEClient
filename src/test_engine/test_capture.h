#pragma once

// Screen capture support for the Dear ImGui Test Engine.
//
// The engine calls this callback to read back the rendered frame so that it
// can write PNG files or record videos.

#include <imgui.h>

namespace app::testing {

/// Reads a region of the default framebuffer into @p pixels (RGBA8, top row
/// first). Returns false when the region could not be captured.
bool captureScreenRegion(ImGuiID viewportId, int x, int y, int w, int h,
                         unsigned int* pixels, void* userData);

} // namespace app::testing
