#include "test_engine/test_capture.h"

#include <GLFW/glfw3.h>

#ifdef __linux__
#include <unistd.h>
#endif

#include <algorithm>
#include <cstring>
#include <vector>

namespace app::testing {

namespace {

// Reads the given region of the bound viewport's default framebuffer and
// writes it into @p pixels as tightly packed RGBA8 rows, top row first.
// Contract (see imgui_test_engine/shared/imgui_app.cpp): the caller must have
// made the viewport's GL context current; a client surface may need that
// context switch explicitly.
bool readFramebufferRegion(const ImGuiViewport* viewport, int x, int y, int w, int h,
                           unsigned int* pixels) {
#ifdef __linux__
    // On some compositors the captured frame would otherwise be outdated.
    usleep(1000);
#endif

    // OpenGL's origin is bottom-left, the capture expects top-left. The
    // reference height is the viewport's own size: under multi-viewport this
    // equals the target surface, under single-viewport it equals DisplaySize.
    const int flippedY = static_cast<int>(viewport->Size.y) - (y + h);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, flippedY, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Flip the rows so that the first row is the top one.
    const size_t stride = static_cast<size_t>(w) * 4;
    std::vector<unsigned char> lineTmp(stride);
    unsigned char* lineA = reinterpret_cast<unsigned char*>(pixels);
    unsigned char* lineB = reinterpret_cast<unsigned char*>(pixels) + stride * (static_cast<size_t>(h) - 1);
    while (lineA < lineB) {
        std::memcpy(lineTmp.data(), lineA, stride);
        std::memcpy(lineA, lineB, stride);
        std::memcpy(lineB, lineTmp.data(), stride);
        lineA += stride;
        lineB -= stride;
    }
    return true;
}

} // namespace

bool captureScreenRegion(ImGuiID viewportId, int x, int y, int w, int h,
                         unsigned int* pixels, void* userData) {
    (void)userData;
    // Switch to the GL context of the requested viewport so a capture stays
    // correct when the engine shoots a secondary platform window: every
    // ImGui viewport owns its own framebuffer (GLFW backend stores the
    // window in PlatformHandle).
    ImGuiViewport* viewport = ImGui::FindViewportByID(viewportId);
    if (!viewport) {
        viewport = ImGui::GetMainViewport();
    }
    auto* target = static_cast<GLFWwindow*>(viewport->PlatformHandle);
    GLFWwindow* previous = glfwGetCurrentContext();
    if (!target) {
        return false;
    }

    const bool switched = (target != previous);
    if (switched) {
        glfwMakeContextCurrent(target);
    }
    const bool ok = readFramebufferRegion(viewport, x, y, w, h, pixels);
    if (switched) {
        glfwMakeContextCurrent(previous);
    }
    return ok;
}

} // namespace app::testing
