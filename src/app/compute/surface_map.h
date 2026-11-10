#pragma once

#include <optional>
#include <vector>

#include "app/compute/sag_sampling.h"
#include "app/models/surface_data.h"
#include "app/models/types.h"

namespace app::compute {

    // Irregularity map limits: the limits live here because both the UI and
    // the service use them: one value, one rule. Each limit follows from its
    // own constraint.
    //
    // The profile point count comes from app/compute/sag_sampling.h - these
    // are Zemax tool limits, identical for the profile and the map.

    // Angular step between sections, degrees. Below 0.001 Zemax cannot tell
    // the difference anyway: double resolution is ~1e-16 radians, ~1e-14 degrees.
    constexpr double MIN_ANGLE_STEP_DEG = 0.001;
    constexpr double MAX_ANGLE_STEP_DEG = 180.0;

    // DDE request budget for one map. A profile costs (sampling + 2) requests:
    // two for the surface itself and one per point. The client keeps a single
    // request in flight, so the time ~ budget x RTT (tens of ms per GetSag).
    //
    // 16M requests at 65 points mean 238 805 sections (step 0.00075), so the
    // advertised 0.001 (180 000 sections, ~12M requests, ~190 MB of profiles)
    // is reachable. Changing this number changes the precision limit and the
    // calculation time.
    //
    // The second speed regulator is the per-request timeout (Preferences ->
    // DDE Performance -> GetSag Map): it sets how long to wait for the server.
    constexpr long long MAX_MAP_REQUESTS = 16'000'000;

    /// Map calculation plan: effective values and load estimate.
    struct MapPlan {
        double    stepDeg = 0.0;           // effective step between sections, degrees
        int       sampling = 0;            // effective profile point count
        int       sections = 0;            // number of sections (profiles)
        long long estimatedRequests = 0;   // DDE request estimate
        /// true means the effective values differ from the entered ones: the
        /// step or the point count had to be raised to the limits.
        bool      adjusted = false;
    };

    /// The map plan for the entered values.
    ///
    /// The step and point count are limited by the bounds, and the step is
    /// additionally raised so the request estimate does not exceed
    /// MAX_MAP_REQUESTS: 0.001 at 65 points is 180 000 sections and ~12M
    /// requests.
    MapPlan planSurfaceMap(double stepDeg, int sampling);

    class SurfaceMap {
        public:
            std::optional<app::models::MaxPVResult> findMaxPVSection(
                const std::vector<app::models::SurfaceData>& profiles,
                const app::models::SurfaceData& nominalSurfaceData) const;
    };

}
