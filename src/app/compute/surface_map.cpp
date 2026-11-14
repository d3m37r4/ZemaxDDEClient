#include "surface_map.h"

#include <algorithm>
#include <cmath>
#include <numbers>
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

    float maxSemiDiameter(const std::vector<app::models::SurfaceData>& profiles) {
        float widest = 0.0f;
        for (const auto& profile : profiles) {
            widest = std::max(widest, static_cast<float>(profile.semiDiameter));
        }
        return widest;
    }

    int drawnProfileStride(int numProfiles, int itemsPerProfile, int maxItems) {
        if (numProfiles <= 0 || itemsPerProfile <= 0 || maxItems <= 0) {
            return 1;
        }
        const int maxProfiles = std::max(1, maxItems / itemsPerProfile);
        return std::max(1, (numProfiles + maxProfiles - 1) / maxProfiles);
    }

    MapMesh buildMapMesh(const std::vector<app::models::SurfaceData>& profiles,
                          int sampling, double semiDiameter,
                          const app::models::SurfaceData* nominal,
                          int maxVertices) {
        MapMesh mesh;
        if (profiles.empty() || sampling < 2 || semiDiameter <= 0.0 || maxVertices <= 0) {
            return mesh;
        }
        const int numRadii = (sampling + 1) / 2;
        const int centerIdx = (sampling - 1) / 2;
        const double rStep = semiDiameter / (numRadii - 1);
        const int numProfiles = static_cast<int>(profiles.size());
        const int numAngles = numProfiles * 2;

        // Decimate the sections so the mesh stays inside the vertex budget.
        const int verticesPerProfile = numRadii * 2;
        const int maxProfiles = std::max(1, maxVertices / std::max(1, verticesPerProfile));
        const int stride = std::max(1, (numProfiles + maxProfiles - 1) / maxProfiles);

        // The kept angles in geometric order: every section contributes two
        // halves (the section itself and its opposite), so the vertices are
        // laid out as all positive halves first and the opposite halves
        // afterwards. Connecting them in section order instead would make the
        // triangles jump between the halves and fan out into spikes.
        std::vector<int> keptAngles;
        for (int k = 0; k < numProfiles; k += stride) {
            keptAngles.push_back(2 * k);
        }
        for (int k = 0; k < numProfiles; k += stride) {
            keptAngles.push_back(2 * k + 1);
        }

        mesh.x.reserve(keptAngles.size() * static_cast<size_t>(numRadii));
        mesh.y.reserve(keptAngles.size() * static_cast<size_t>(numRadii));
        mesh.z.reserve(keptAngles.size() * static_cast<size_t>(numRadii));

        for (const int j : keptAngles) {
            const int profileIdx = std::min(j / 2, numProfiles - 1);
            const bool positiveHalf = (j % 2 == 0);
            const auto& points = profiles[profileIdx].sagDataPoints;
            if (points.empty()) continue;
            double angleRad = profileIdx * std::numbers::pi / numProfiles;
            if (!positiveHalf) angleRad += std::numbers::pi;
            for (int i = 0; i < numRadii; ++i) {
                const double r = i * rStep;
                mesh.x.push_back(static_cast<float>(r * std::cos(angleRad)));
                mesh.y.push_back(static_cast<float>(r * std::sin(angleRad)));
                int srcIdx = positiveHalf ? centerIdx + i : centerIdx - i;
                srcIdx = std::max(0, std::min(static_cast<int>(points.size()) - 1, srcIdx));
                float value = static_cast<float>(points[srcIdx].sag);
                if (nominal != nullptr && srcIdx < static_cast<int>(nominal->sagDataPoints.size())) {
                    value -= static_cast<float>(nominal->sagDataPoints[srcIdx].sag);
                }
                mesh.z.push_back(value);
            }
        }

        // Connect the kept angles into triangles, closing the ring so the
        // surface has no seam. The innermost ring collapses into the centre,
        // so its cells are skipped.
        const int kept = static_cast<int>(keptAngles.size());
        for (int a = 0; a < kept; ++a) {
            const int next = (a + 1) % kept;
            const unsigned int base0 = static_cast<unsigned int>(a * numRadii);
            const unsigned int base1 = static_cast<unsigned int>(next * numRadii);
            // The innermost cells fan out from the shared centre vertex, so
            // they are real triangles - skipping them left a hole in the middle.
            for (int i = 0; i + 1 < numRadii; ++i) {
                mesh.indices.push_back(base0 + static_cast<unsigned int>(i));
                mesh.indices.push_back(base0 + static_cast<unsigned int>(i + 1));
                mesh.indices.push_back(base1 + static_cast<unsigned int>(i));
                mesh.indices.push_back(base1 + static_cast<unsigned int>(i));
                mesh.indices.push_back(base0 + static_cast<unsigned int>(i + 1));
                mesh.indices.push_back(base1 + static_cast<unsigned int>(i + 1));
            }
        }
        return mesh;
    }

}
