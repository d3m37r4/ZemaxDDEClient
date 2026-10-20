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

// Reads the given region of the default framebuffer and writes it into
// @p pixels as tightly packed RGBA8 rows, top row first.
bool readFramebufferRegion(int x, int y, int w, int h, unsigned int* pixels) {
#ifdef __linux__
    // On some compositors the captured frame would otherwise be outdated.
    usleep(1000);
#endif

    // OpenGL's origin is bottom-left, the capture expects top-left.
    const int flippedY = static_cast<int>(ImGui::GetIO().DisplaySize.y) - (y + h);

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
    (void)viewportId;
    (void)userData;
    return readFramebufferRegion(x, y, w, h, pixels);
}

} // namespace app::testing
