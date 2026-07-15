#include "surface_map.h"

namespace app::compute {

    std::optional<app::models::MaxPVResult> SurfaceMap::findMaxPVSection(
        const std::vector<app::models::SurfaceData>& profiles,
        const app::models::SurfaceData& nominalSurfaceData) const
    {
        if (profiles.empty() || !nominalSurfaceData.isValid()) return std::nullopt;
        if (profiles[0].sagDataPoints.size() != nominalSurfaceData.sagDataPoints.size()) return std::nullopt;

        std::optional<app::models::MaxPVResult> best;
        size_t numPoints = nominalSurfaceData.sagDataPoints.size();

        for (const auto& profile : profiles) {
            if (profile.sagDataPoints.size() != numPoints) continue;

            double P = -1e30, V = 1e30;
            for (size_t i = 0; i < numPoints; ++i) {
                double delta = profile.sagDataPoints[i].sag - nominalSurfaceData.sagDataPoints[i].sag;
                if (delta > P) P = delta;
                if (delta < V) V = delta;
            }
            double pv = P - V;

            if (!best.has_value() || pv > best->pv) {
                best = app::models::MaxPVResult{profile.angle, P, V, pv};
            }
        }

        return best;
    }

}
