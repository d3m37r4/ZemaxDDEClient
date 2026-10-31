#include <algorithm>
#include <cctype>
#include <cstring>
#include <format>

#include "gui/plot_image_export_util.h"
#include "app/models/surface_data.h"

namespace gui::plot_export {

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

    ImageFormat imageFormatFromPath(const std::string& path) {
        const size_t dotPos = path.find_last_of('.');
        const size_t slashPos = path.find_last_of("/\\");
        if (dotPos == std::string::npos ||
            (slashPos != std::string::npos && dotPos < slashPos)) {
            return ImageFormat::Unknown;
        }
        std::string ext = path.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext == "png") return ImageFormat::Png;
        if (ext == "jpg" || ext == "jpeg") return ImageFormat::Jpeg;
        if (ext == "bmp") return ImageFormat::Bmp;
        return ImageFormat::Unknown;
    }

    void flipRows(std::vector<unsigned char>& pixels, int width, int height) {
        if (width <= 0 || height <= 0) {
            return;
        }
        const size_t stride = static_cast<size_t>(width) * 4;
        if (pixels.size() < stride * static_cast<size_t>(height)) {
            return;
        }
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

}
