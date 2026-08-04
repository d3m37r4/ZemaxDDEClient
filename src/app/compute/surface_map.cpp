#include "surface_map.h"

#include <cmath>
#include <optional>

namespace app::compute {

    std::optional<app::models::MaxPVResult> SurfaceMap::findMaxPVSection(
        const std::vector<app::models::SurfaceData>& profiles,
        const app::models::SurfaceData& nominalSurfaceData) const
    {
        if (profiles.empty() || !nominalSurfaceData.isValid()) return std::nullopt;
        if (profiles[0].sagDataPoints.empty()) return std::nullopt;
        size_t numPoints = profiles[0].sagDataPoints.size();
        if (nominalSurfaceData.sagDataPoints.size() != numPoints) return std::nullopt;

        std::optional<app::models::MaxPVResult> best;

        for (const auto& profile : profiles) {
            if (profile.sagDataPoints.size() != numPoints) continue;

            // Track whether any sample in this section is non-finite; if so the
            // section contributes a garbage P-V and must be skipped entirely.
            bool sectionFinite = true;
            double P = -1e30, V = 1e30;
            for (size_t i = 0; i < numPoints; ++i) {
                double delta = profile.sagDataPoints[i].sag - nominalSurfaceData.sagDataPoints[i].sag;
                if (!std::isfinite(delta)) {
                    sectionFinite = false;
                    break;
                }
                if (delta > P) P = delta;
                if (delta < V) V = delta;
            }
            if (!sectionFinite) continue;

            double pv = P - V;
            if (!std::isfinite(pv)) continue;

            if (!best.has_value() || pv > best->pv) {
                best = app::models::MaxPVResult{profile.angle, P, V, pv};
            }
        }

        return best;
    }

}
