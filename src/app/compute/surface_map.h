#pragma once

#include <optional>
#include <vector>

#include "app/models/surface_data.h"
#include "app/models/types.h"

namespace app::compute {

    class SurfaceMap {
        public:
            std::optional<app::models::MaxPVResult> findMaxPVSection(
                const std::vector<app::models::SurfaceData>& profiles,
                const app::models::SurfaceData& nominalSurfaceData) const;
    };

}
