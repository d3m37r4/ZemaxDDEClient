#include "surface_map.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace app::compute {

    MapPlan planSurfaceMap(double stepDeg, int sampling) {
        MapPlan plan;

        int points = sampling;
        if (points < MIN_SAG_SAMPLING) points = MIN_SAG_SAMPLING;
        if (points > MAX_SAG_SAMPLING) points = MAX_SAG_SAMPLING;
        if (points != sampling) plan.adjusted = true;

        double step = stepDeg;
        if (step < MIN_ANGLE_STEP_DEG) {
            step = MIN_ANGLE_STEP_DEG;
            plan.adjusted = true;
        }
        if (step > MAX_ANGLE_STEP_DEG) {
            step = MAX_ANGLE_STEP_DEG;
            plan.adjusted = true;
        }

        int sections = std::max(1, static_cast<int>(180.0 / step));

        // One profile costs (sampling + 2) requests: two for the surface data and
        // one per sag point. Keep the total inside the budget.
        const long long requestsPerSection = points + 2;
        const int maxSections = std::max(
            1, static_cast<int>(MAX_MAP_REQUESTS / requestsPerSection));
        if (sections > maxSections) {
            sections = maxSections;
            step = 180.0 / sections;
            plan.adjusted = true;
        }

        plan.stepDeg = step;
        plan.sampling = points;
        plan.sections = sections;
        plan.estimatedRequests = static_cast<long long>(sections) * requestsPerSection;
        return plan;
    }

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
