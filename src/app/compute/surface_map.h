#pragma once

#include <cmath>
#include <numbers>
#include <optional>
#include <vector>

#include "app/models/surface_data.h"
#include "app/models/types.h"

namespace app::compute {

    struct SurfaceMatrices {
        std::vector<float> X, Y, Z;
        float zMin{0}, zMax{0};
    };

    class SurfaceMap {
        public:
            SurfaceMatrices buildSurfaceMatrices(
                const std::vector<app::models::SurfaceData>& profiles,
                const app::models::SurfaceData& nominalSurfaceData,
                int targetSampling, int totalAngles, bool deviation) const;

            std::optional<app::models::MaxPVResult> findMaxPVSection(
                const std::vector<app::models::SurfaceData>& profiles,
                const app::models::SurfaceData& nominalSurfaceData) const;
    };

}
