#include "surface_map.h"

namespace app::compute {

    SurfaceMatrices SurfaceMap::buildSurfaceMatrices(
        const std::vector<app::models::SurfaceData>& profiles,
        const app::models::SurfaceData& nominalSurfaceData,
        int targetSampling, int totalAngles, bool deviation) const
    {
        SurfaceMatrices result;
        if (profiles.empty()) return result;

        int numRadii = (targetSampling + 1) / 2;
        int numAngles = totalAngles * 2;
        int centerIdx = (targetSampling - 1) / 2;
        double semiDiameter = profiles[0].semiDiameter;

        result.X.resize(static_cast<size_t>(numRadii * numAngles));
        result.Y.resize(static_cast<size_t>(numRadii * numAngles));
        result.Z.resize(static_cast<size_t>(numRadii * numAngles));
        bool first = true;

        int numProfiles = static_cast<int>(profiles.size());

        for (int j = 0; j < numAngles; ++j) {
            int profileIdx = std::min(j / 2, numProfiles - 1);
            bool positiveHalf = (j % 2 == 0);
            double angleRad = profileIdx * std::numbers::pi / numProfiles;
            if (!positiveHalf) angleRad += std::numbers::pi;

            double rStep = semiDiameter / (numRadii - 1);

            for (int i = 0; i < numRadii; ++i) {
                double r = i * rStep;
                size_t idx = static_cast<size_t>(i * numAngles + j);
                result.X[idx] = static_cast<float>(r * std::cos(angleRad));
                result.Y[idx] = static_cast<float>(r * std::sin(angleRad));

                int srcIdx = positiveHalf ? centerIdx + i : centerIdx - i;
                srcIdx = std::max(0, std::min(static_cast<int>(profiles[profileIdx].sagDataPoints.size()) - 1, srcIdx));

                float sag = static_cast<float>(profiles[profileIdx].sagDataPoints[srcIdx].sag);

                if (deviation && nominalSurfaceData.isValid() && srcIdx < static_cast<int>(nominalSurfaceData.sagDataPoints.size())) {
                    sag -= static_cast<float>(nominalSurfaceData.sagDataPoints[srcIdx].sag);
                }

                result.Z[idx] = sag;

                if (first) { result.zMin = sag; result.zMax = sag; first = false; }
                else { result.zMin = std::min(result.zMin, sag); result.zMax = std::max(result.zMax, sag); }
            }
        }

        return result;
    }

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
