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

        for (size_t index = 0; index < profiles.size(); ++index) {
            const auto& profile = profiles[index];
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
                best = app::models::MaxPVResult{profile.angle, P, V, pv,
                                               static_cast<int>(index)};
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
        const double rStep = semiDiameter / (numRadii - 1);
        const int numProfiles = static_cast<int>(profiles.size());

        // ImPlot3D's RendererTriangleFill unrolls every triangle into three
        // separate ImDrawVert vertices in a 16-bit ImDrawIdx buffer, so the
        // budget counts unrolled vertices and not mesh vertices: one angular
        // cell (1 angle step x 1 radial step) costs 6 of them (2 triangles),
        // one profile (2 halves) with R radial steps costs 12 * R.
        // Decimating the radii first keeps the angular resolution of the map
        // (every section stays visible) and only then thins the sections.
        constexpr int kMinRadialSteps = 16;
        const int totalRadialSteps = std::max(1, numRadii - 1);
        int radialStride = 1;
        const long long fullCost = 12LL * numProfiles * totalRadialSteps;
        if (fullCost > maxVertices && totalRadialSteps > kMinRadialSteps) {
            const int neededStride = static_cast<int>((fullCost + maxVertices - 1) / maxVertices);
            const int maxRadialStride = std::max(1, totalRadialSteps / kMinRadialSteps);
            radialStride = std::clamp(neededStride, 1, maxRadialStride);
        }

        std::vector<int> keptRadii;
        for (int i = 0; i < numRadii - 1; i += radialStride) {
            keptRadii.push_back(i);
        }
        keptRadii.push_back(numRadii - 1);
        const int meshRadii = static_cast<int>(keptRadii.size());
        const int radialSteps = std::max(1, meshRadii - 1);

        // Each kept profile (its two halves) costs 12 * radialSteps vertices.
        const int verticesPerProfile = 12 * radialSteps;
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

        mesh.x.reserve(keptAngles.size() * static_cast<size_t>(meshRadii));
        mesh.y.reserve(keptAngles.size() * static_cast<size_t>(meshRadii));
        mesh.z.reserve(keptAngles.size() * static_cast<size_t>(meshRadii));

        for (const int j : keptAngles) {
            const int profileIdx = std::min(j / 2, numProfiles - 1);
            const bool positiveHalf = (j % 2 == 0);
            const auto& points = profiles[profileIdx].sagDataPoints;
            if (points.empty()) continue;
            // The centre belongs to the section's own data: a section may end
            // up with fewer points than the sampling setting.
            const int profileCenter = (static_cast<int>(points.size()) - 1) / 2;
            double angleRad = profileIdx * std::numbers::pi / numProfiles;
            if (!positiveHalf) angleRad += std::numbers::pi;
            for (const int i : keptRadii) {
                const double r = i * rStep;
                mesh.x.push_back(static_cast<float>(r * std::cos(angleRad)));
                mesh.y.push_back(static_cast<float>(r * std::sin(angleRad)));
                int srcIdx = positiveHalf ? profileCenter + i : profileCenter - i;
                srcIdx = std::max(0, std::min(static_cast<int>(points.size()) - 1, srcIdx));
                float value = static_cast<float>(points[srcIdx].sag);
                if (nominal != nullptr && srcIdx < static_cast<int>(nominal->sagDataPoints.size())) {
                    value -= static_cast<float>(nominal->sagDataPoints[srcIdx].sag);
                }
                mesh.z.push_back(value);
            }
        }

        // Connect the kept angles into triangles, closing the ring so the
        // surface has no seam. The innermost cells fan out from the shared
        // centre vertex, so they are real triangles - skipping them left a
        // hole in the middle.
        const int kept = static_cast<int>(keptAngles.size());
        for (int a = 0; a < kept; ++a) {
            const int next = (a + 1) % kept;
            const unsigned int base0 = static_cast<unsigned int>(a * meshRadii);
            const unsigned int base1 = static_cast<unsigned int>(next * meshRadii);
            for (int i = 0; i + 1 < meshRadii; ++i) {
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
