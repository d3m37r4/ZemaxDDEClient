#include <algorithm>
#include <cctype>
#include <cstring>
#include <exception>
#include <format>
#include <vector>

#include <nfd.h>

#include "lib/imgui/imgui.h"
#include "lib/imgui/backends/imgui_impl_opengl3.h"
// Declarations only: the entry points come from imgui_impl_opengl3.cpp's own
// IMGL3W_IMPL instance (extern imgl3wProcs) - defining the impl here would
// duplicate every symbol.
#include "lib/imgui/backends/imgui_impl_opengl3_loader.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/imgui_test_engine/imgui_test_engine/thirdparty/stb/imstb_image_write.h"

#include "gui/plot_image_exporter.h"
#include "gui/imgui_utils.h"
#include "app/models/surface_data.h"
#include "logger/logger.h"

// Framebuffer enums/entry points are absent from the vendored loader (it is
// filtered to what imgui_impl_opengl3.cpp itself uses), so the exporter keeps
// its own copies and resolves the functions at first use.
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_PACK_ALIGNMENT
#define GL_PACK_ALIGNMENT 0x0D05
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

namespace gui::plot_export {
    namespace {
        constexpr int kMinExportSide = 16;
        constexpr int kMaxExportSide = 16384;
        // 64 MP keeps a single RGBA readback buffer under 256 MB.
        constexpr long long kMaxExportPixels = 64LL * 1024 * 1024;

        using GenFramebuffersFn = void (APIENTRY*)(GLsizei, GLuint*);
        using BindFramebufferFn = void (APIENTRY*)(GLenum, GLuint);
        using FramebufferTexture2DFn = void (APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);
        using CheckFramebufferStatusFn = GLenum (APIENTRY*)(GLenum);
        using DeleteFramebuffersFn = void (APIENTRY*)(GLsizei, const GLuint*);

        struct FramebufferApi {
            GenFramebuffersFn gen = nullptr;
            BindFramebufferFn bind = nullptr;
            FramebufferTexture2DFn texture = nullptr;
            CheckFramebufferStatusFn check = nullptr;
            DeleteFramebuffersFn destroy = nullptr;
        };

        FramebufferApi& framebufferApi() {
            static FramebufferApi api = [] {
                FramebufferApi loaded;
                loaded.gen = reinterpret_cast<GenFramebuffersFn>(
                    imgl3wGetProcAddress("glGenFramebuffers"));
                loaded.bind = reinterpret_cast<BindFramebufferFn>(
                    imgl3wGetProcAddress("glBindFramebuffer"));
                loaded.texture = reinterpret_cast<FramebufferTexture2DFn>(
                    imgl3wGetProcAddress("glFramebufferTexture2D"));
                loaded.check = reinterpret_cast<CheckFramebufferStatusFn>(
                    imgl3wGetProcAddress("glCheckFramebufferStatus"));
                loaded.destroy = reinterpret_cast<DeleteFramebuffersFn>(
                    imgl3wGetProcAddress("glDeleteFramebuffers"));
                return loaded;
            }();
            return api;
        }

        Job g_job;
        bool g_hasJob = false;

        GLuint g_fbo = 0;
        GLuint g_texture = 0;
        int g_targetWidth = 0;
        int g_targetHeight = 0;

        void destroyTarget() {
            FramebufferApi& api = framebufferApi();
            if (g_fbo && api.destroy) {
                api.destroy(1, &g_fbo);
            }
            if (g_texture) {
                glDeleteTextures(1, &g_texture);
            }
            g_fbo = 0;
            g_texture = 0;
            g_targetWidth = 0;
            g_targetHeight = 0;
        }

        bool ensureTarget(int width, int height) {
            FramebufferApi& api = framebufferApi();
            if (!api.gen || !api.bind || !api.texture || !api.check || !api.destroy) {
                return false;
            }
            if (g_fbo && g_targetWidth == width && g_targetHeight == height) {
                return true;
            }
            destroyTarget();

            glGenTextures(1, &g_texture);
            glBindTexture(GL_TEXTURE_2D, g_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            api.gen(1, &g_fbo);
            api.bind(GL_FRAMEBUFFER, g_fbo);
            api.texture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_texture, 0);
            const bool complete = api.check(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
            api.bind(GL_FRAMEBUFFER, 0);
            if (!complete) {
                destroyTarget();
                return false;
            }
            g_targetWidth = width;
            g_targetHeight = height;
            return true;
        }

        // glReadPixels returns bottom-up rows; the encoders expect top-down.
        void flipRows(std::vector<unsigned char>& pixels, int width, int height) {
            const size_t stride = static_cast<size_t>(width) * 4;
            std::vector<unsigned char> line(stride, 0);
            unsigned char* top = pixels.data();
            unsigned char* bottom = pixels.data() + stride * static_cast<size_t>(height - 1);
            for (int i = 0; i < height / 2; ++i) {
                std::memcpy(line.data(), top, stride);
                std::memcpy(top, bottom, stride);
                std::memcpy(bottom, line.data(), stride);
                top += stride;
                bottom -= stride;
            }
        }

        bool writeImage(const std::string& path, int width, int height,
                        const std::vector<unsigned char>& pixels) {
            const size_t dotPos = path.find_last_of('.');
            const size_t slashPos = path.find_last_of("/\\");
            std::string ext;
            if (dotPos != std::string::npos && (slashPos == std::string::npos || dotPos > slashPos)) {
                ext = path.substr(dotPos + 1);
            }
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            const int stride = width * 4;
            if (ext == "png") {
                return stbi_write_png(path.c_str(), width, height, 4, pixels.data(), stride) == 1;
            }
            if (ext == "jpg" || ext == "jpeg") {
                return stbi_write_jpg(path.c_str(), width, height, 4, pixels.data(), 90) == 1;
            }
            if (ext == "bmp") {
                return stbi_write_bmp(path.c_str(), width, height, 4, pixels.data()) == 1;
            }
            return false;
        }
    }

    void queueJob(const Job& job) {
        g_job = job;
        g_hasJob = true;
    }

    void renderPendingJob() {
        if (!g_hasJob) {
            return;
        }
        Job job = g_job;
        g_hasJob = false;

        auto log = [&job](std::string message) {
            if (job.logger) {
                job.logger->addLog(message);
            }
        };

        if (!job.drawList) {
            log("[GUI] Plot image export skipped: no draw list captured");
            return;
        }
        if (job.displaySize.x <= 0.0f || job.displaySize.y <= 0.0f) {
            log("[GUI] Plot image export skipped: empty plot rect");
            return;
        }
        const int width = std::clamp(job.targetWidth, kMinExportSide, kMaxExportSide);
        const int height = std::clamp(job.targetHeight, kMinExportSide, kMaxExportSide);
        if (static_cast<long long>(width) * height > kMaxExportPixels) {
            log(std::format("[GUI] Plot image export skipped: {}x{} exceeds the pixel budget",
                            width, height));
            return;
        }
        if (!ensureTarget(width, height)) {
            log(std::format("[GUI] Plot image export failed: cannot create {}x{} offscreen target",
                            width, height));
            return;
        }

        GLint previousFbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);
        framebufferApi().bind(GL_FRAMEBUFFER, g_fbo);
        glViewport(0, 0, width, height);
        const ImVec4 background = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        glClearColor(background.x, background.y, background.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImDrawData drawData;
        drawData.Valid = true;
        drawData.AddDrawList(job.drawList);
        drawData.DisplayPos = job.displayPos;
        drawData.DisplaySize = job.displaySize;
        drawData.FramebufferScale = ImVec2(static_cast<float>(width) / job.displaySize.x,
                                           static_cast<float>(height) / job.displaySize.y);
        drawData.Textures = ImGui::GetDrawData() ? ImGui::GetDrawData()->Textures : nullptr;
        ImGui_ImplOpenGL3_RenderDrawData(&drawData);

        std::vector<unsigned char> pixels(
            static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 0);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        framebufferApi().bind(GL_FRAMEBUFFER, static_cast<GLuint>(previousFbo));
        flipRows(pixels, width, height);

        if (!writeImage(job.targetPath, width, height, pixels)) {
            log(std::format("[GUI] Plot image write failed: {}", job.targetPath));
            return;
        }
        log(std::format("[GUI] Plot image saved to {} ({}x{})", job.targetPath, width, height));
    }

    std::string defaultImageName(const char* kind, const app::models::SurfaceData& surface) {
        std::string angle = std::format("{:.2f}", surface.angle);
        if (angle.find('.') != std::string::npos) {
            while (angle.back() == '0') angle.pop_back();
            if (angle.back() == '.') angle.pop_back();
        }
        std::string name = std::format("{}_surf{}_{}deg", kind, surface.id, angle);
        if (surface.sourceSlot >= 0) {
            name += std::format("_slot{}", surface.sourceSlot);
        }
        return name;
    }

    bool chooseSavePath(Logger& logger, const std::string& defaultName, std::string& outPath) {
        static const nfdu8filteritem_t kFormats[] = {
            {"PNG image", "png"},
            {"JPEG image", "jpg,jpeg"},
            {"Windows Bitmap", "bmp"},
        };
        const nfdsavedialogu8args_t args = {
            kFormats, static_cast<nfdfiltersize_t>(IM_ARRAYSIZE(kFormats)),
            nullptr, defaultName.c_str(), {}, nullptr, nullptr, nullptr};
        nfdu8char_t* path = nullptr;
        const nfdresult_t result = NFD_SaveDialogU8_With(&path, &args);
        if (result == NFD_CANCEL) {
            return false;
        }
        if (result == NFD_ERROR || !path) {
            logger.addLog(std::format("[GUI] Save dialog failed: {}",
                                      NFD_GetError() ? NFD_GetError() : "unknown NFD error"));
            return false;
        }
        outPath = reinterpret_cast<const char*>(path);
        NFD_FreePathU8(path);

        // The dialog may return an extensionless path (file type set via the
        // filter dropdown); fall back to PNG, the encoder follows the
        // extension resolved in writeImage().
        const size_t slashPos = outPath.find_last_of("/\\");
        const size_t dotPos = outPath.find_last_of('.');
        const bool hasExtension = dotPos != std::string::npos &&
                                  (slashPos == std::string::npos || dotPos > slashPos);
        if (!hasExtension) {
            outPath += ".png";
        }
        return true;
    }

    void requestSave(Logger& logger, const char* kind, const app::models::SurfaceData& surface,
                     ImDrawList* drawList, const ImVec2& plotPos, const ImVec2& plotSize) {
        try {
            const ImVec2 resolution = ImGuiUtils::PlotExportResolution();
            std::string path;
            if (!chooseSavePath(logger, defaultImageName(kind, surface), path)) {
                return;
            }
            Job job;
            job.drawList = drawList;
            job.displayPos = plotPos;
            job.displaySize = plotSize;
            job.targetWidth = static_cast<int>(resolution.x);
            job.targetHeight = static_cast<int>(resolution.y);
            job.targetPath = std::move(path);
            job.logger = &logger;
            queueJob(job);
        } catch (const std::exception& ex) {
            logger.addLog(std::format("[GUI] Plot image save threw: {}", ex.what()));
        }
    }
}
