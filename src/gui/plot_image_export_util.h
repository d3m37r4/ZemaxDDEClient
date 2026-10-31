#pragma once

#include <string>
#include <vector>

namespace app::models { struct SurfaceData; }

namespace gui::plot_export {

    enum class ImageFormat { Png, Jpeg, Bmp, Unknown };

    /// Default save-dialog name: "<kind>_surf<id>_<angle>deg[_slot<N>]".
    /// Trailing zeros of the angle are trimmed (0.00 -> 0, 22.50 -> 22.5);
    /// the slot suffix is dropped when the data has no DDE slot (-1).
    std::string defaultImageName(const char* kind, const app::models::SurfaceData& surface);

    /// Format picked from the final path extension (case-insensitive).
    ImageFormat imageFormatFromPath(const std::string& path);

    /// Converts a bottom-up RGBA8 buffer (glReadPixels order) to top-down in
    /// place - the encoders expect the top row first.
    void flipRows(std::vector<unsigned char>& pixels, int width, int height);

}
