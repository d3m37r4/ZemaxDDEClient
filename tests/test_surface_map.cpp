#include <gtest/gtest.h>

#include <limits>
#include <cmath>
#include <numbers>
#include <vector>

#include "app/compute/surface_map.h"
#include "app/models/surface_data.h"

namespace app::compute {
namespace {

app::models::SurfaceData makeProfile(double angle, std::vector<double> sags) {
    app::models::SurfaceData sd;
    sd.id = 0;
    sd.angle = angle;
    for (double s : sags) {
        app::models::SagData pt;
        pt.sag = s;
        sd.sagDataPoints.push_back(pt);
    }
    return sd;
}

TEST(SurfaceMap, EmptyProfilesReturnsNullopt) {
    SurfaceMap map;
    app::models::SurfaceData nominal = makeProfile(0.0, {0.0, 0.1, 0.0});
    EXPECT_FALSE(map.findMaxPVSection({}, nominal).has_value());
}

TEST(SurfaceMap, InvalidNominalReturnsNullopt) {
    SurfaceMap map;
    app::models::SurfaceData nominal; // default id == -1 => invalid
    auto profiles = { makeProfile(0.0, {0.0, 0.1}) };
    EXPECT_FALSE(map.findMaxPVSection(profiles, nominal).has_value());
}

TEST(SurfaceMap, PointCountMismatchReturnsNullopt) {
    SurfaceMap map;
    app::models::SurfaceData nominal = makeProfile(0.0, {0.0, 0.1, 0.2});
    auto profiles = { makeProfile(0.0, {0.0, 0.1}) };
    EXPECT_FALSE(map.findMaxPVSection(profiles, nominal).has_value());
}

TEST(SurfaceMap, FindsWorstPVSection) {
    SurfaceMap map;
    app::models::SurfaceData nominal = makeProfile(0.0, {0.0, 0.5, 0.0});

    // Section A: deltas {0, +1, 0} -> PV = 1
    // Section B: deltas {0, +2, 0} -> PV = 2 (worst)
    std::vector<app::models::SurfaceData> profiles;
    profiles.push_back(makeProfile(0.0, {0.0, 1.5, 0.0}));
    profiles.push_back(makeProfile(90.0, {0.0, 2.5, 0.0}));

    auto best = map.findMaxPVSection(profiles, nominal);
    ASSERT_TRUE(best.has_value());
    EXPECT_EQ(best->angle, 90.0);
    EXPECT_DOUBLE_EQ(best->peak, 2.0);
    EXPECT_DOUBLE_EQ(best->valley, 0.0);
    EXPECT_DOUBLE_EQ(best->pv, 2.0);
}

TEST(SurfaceMap, NatNInfManSkipsSectionRatherThanPoisonResult) {
    SurfaceMap map;
    app::models::SurfaceData nominal = makeProfile(0.0, {0.0, 0.5, 0.0});

    // Section A has clean data with a sane PV; section B contains NaN so its
    // garbage P-V must be disregarded, not treated as the worst.
    app::models::SurfaceData clean    = makeProfile(10.0, {0.0, 1.5, 0.0});
    app::models::SurfaceData poisoned = makeProfile(20.0, {0.0, std::numeric_limits<double>::quiet_NaN(), 0.0});

    auto best = map.findMaxPVSection({poisoned, clean}, nominal);
    ASSERT_TRUE(best.has_value());
    EXPECT_EQ(best->angle, 10.0);
    EXPECT_DOUBLE_EQ(best->peak, 1.0);
    EXPECT_DOUBLE_EQ(best->pv, 1.0);
}

TEST(SurfaceMap, InfinitySectionIsSkipped) {
    SurfaceMap map;
    app::models::SurfaceData nominal = makeProfile(0.0, {0.0, 0.5, 0.0});

    app::models::SurfaceData plusInf = makeProfile(30.0, {0.0, std::numeric_limits<double>::infinity(), 0.0});
    app::models::SurfaceData good    = makeProfile(40.0, {0.0, 1.5, 0.0});

    auto best = map.findMaxPVSection({plusInf, good}, nominal);
    ASSERT_TRUE(best.has_value());
    EXPECT_EQ(best->angle, 40.0);
}

TEST(SurfaceMapTest, AllSectionsNonFiniteReturnsNullopt) {
    SurfaceMap map;
    app::models::SurfaceData nominal = makeProfile(0.0, {0.0, 0.5, 0.0});

    app::models::SurfaceData badA = makeProfile(1.0, {0.0, std::numeric_limits<double>::infinity(), 0.0});
    app::models::SurfaceData badB = makeProfile(2.0, {0.0, std::numeric_limits<double>::quiet_NaN(), 0.0});

    EXPECT_FALSE(map.findMaxPVSection({badA, badB}, nominal).has_value());
}

TEST(MapPlan, KeepsCoarseStepAndCountsRequests) {
    auto plan = planSurfaceMap(1.0, 65);

    EXPECT_DOUBLE_EQ(plan.stepDeg, 1.0);
    EXPECT_EQ(plan.sampling, 65);
    EXPECT_EQ(plan.sections, 180);
    EXPECT_EQ(plan.estimatedRequests, 180LL * (65 + 2));
    EXPECT_FALSE(plan.adjusted);
}

TEST(MapPlan, KeepsTheCurrentMinimumStepWithinTheBudget) {
    auto plan = planSurfaceMap(0.05, 65);

    EXPECT_DOUBLE_EQ(plan.stepDeg, 0.05);
    EXPECT_EQ(plan.sections, 3600);
    EXPECT_LE(plan.estimatedRequests, MAX_MAP_REQUESTS);
    EXPECT_FALSE(plan.adjusted);
}

TEST(MapPlan, FinestStepFitsTheBudgetAtTypicalSampling) {
    auto plan = planSurfaceMap(MIN_ANGLE_STEP_DEG, 65);

    EXPECT_DOUBLE_EQ(plan.stepDeg, MIN_ANGLE_STEP_DEG);
    EXPECT_EQ(plan.sections, 180000);
    EXPECT_LE(plan.estimatedRequests, MAX_MAP_REQUESTS);
    EXPECT_FALSE(plan.adjusted);
}

TEST(MapPlan, RaisesFineStepToFitTheRequestBudget) {
    // 0.001 deg at 16385 points is 2.9B requests - the budget cannot hold it.
    auto plan = planSurfaceMap(MIN_ANGLE_STEP_DEG, MAX_SAG_SAMPLING);

    EXPECT_GT(plan.stepDeg, MIN_ANGLE_STEP_DEG);
    EXPECT_LE(plan.estimatedRequests, MAX_MAP_REQUESTS);
    EXPECT_TRUE(plan.adjusted);
}

TEST(MapPlan, RaisesStepBelowTheMinimum) {
    auto plan = planSurfaceMap(0.0, 65);

    EXPECT_GT(plan.stepDeg, 0.0);
    EXPECT_LE(plan.estimatedRequests, MAX_MAP_REQUESTS);
    EXPECT_TRUE(plan.adjusted);
}

TEST(MapPlan, ClampsSamplingToTheToolLimit) {
    auto plan = planSurfaceMap(1.0, 100000);

    EXPECT_EQ(plan.sampling, MAX_SAG_SAMPLING);
    EXPECT_TRUE(plan.adjusted);
}

TEST(MapPlan, KeepsTheFullToolRangeOfSampling) {
    // 16385 points is the Zemax tool limit - the plan does not cut it.
    auto plan = planSurfaceMap(1.0, MAX_SAG_SAMPLING);

    EXPECT_EQ(plan.sampling, MAX_SAG_SAMPLING);
    EXPECT_FALSE(plan.adjusted);
}

TEST(MapPlan, RejectsStepAboveHalfTurn) {
    auto plan = planSurfaceMap(200.0, 65);

    EXPECT_DOUBLE_EQ(plan.stepDeg, MAX_ANGLE_STEP_DEG);
    EXPECT_TRUE(plan.adjusted);
}

TEST(SemiDiameter, TakesTheWidestSection) {
    std::vector<app::models::SurfaceData> profiles(3);
    profiles[0].semiDiameter = 0.0;   // a section without a radius
    profiles[1].semiDiameter = 5.5;
    profiles[2].semiDiameter = 4.25;
    EXPECT_FLOAT_EQ(maxSemiDiameter(profiles), 5.5f);
}

TEST(SemiDiameter, NoProfilesMeansNoRadius) {
    EXPECT_FLOAT_EQ(maxSemiDiameter({}), 0.0f);
}

TEST(SemiDiameter, NegativeRadiusIsIgnored) {
    std::vector<app::models::SurfaceData> profiles(2);
    profiles[0].semiDiameter = -1.0;
    profiles[1].semiDiameter = 3.0;
    EXPECT_FLOAT_EQ(maxSemiDiameter(profiles), 3.0f);
}

TEST(ProfileStride, KeepsEverythingWhenItFitsTheBudget) {
    EXPECT_EQ(drawnProfileStride(10, 12, 1200), 1);
    EXPECT_EQ(drawnProfileStride(100, 12, 1200), 1);
}

TEST(ProfileStride, ThinsByTheItemBudget) {
    // 180 sections of 12 items each do not fit into 1200 items.
    EXPECT_EQ(drawnProfileStride(180, 12, 1200), 2);
    EXPECT_EQ(drawnProfileStride(180000, 12, 1200), 1800);
}

TEST(ProfileStride, NeverZeroAndNeverDropsEverything) {
    EXPECT_EQ(drawnProfileStride(0, 12, 1200), 1);
    EXPECT_EQ(drawnProfileStride(180, 0, 1200), 1);
    EXPECT_EQ(drawnProfileStride(180, 12, 0), 1);
    // A budget below one section still leaves one section drawn.
    EXPECT_EQ(drawnProfileStride(5, 12, 4), 5);
}

namespace {
    std::vector<app::models::SurfaceData> makeProfiles(size_t count, double sag = 0.01) {
        std::vector<app::models::SurfaceData> profiles(count);
        for (auto& profile : profiles) {
            profile.semiDiameter = 5.0;
            profile.sagDataPoints.resize(33);
            for (size_t i = 0; i < profile.sagDataPoints.size(); ++i) {
                profile.sagDataPoints[i].sag = sag * static_cast<double>(i);
            }
        }
        return profiles;
    }
} // namespace

TEST(MapMesh, EmptyDataGivesNoMesh) {
    EXPECT_TRUE(buildMapMesh({}, 33, 5.0, nullptr, 1000).empty());
    EXPECT_TRUE(buildMapMesh(makeProfiles(4), 33, 0.0, nullptr, 1000).empty());
}

TEST(MapMesh, VerticesAndIndicesAreConsistent) {
    const MapMesh mesh = buildMapMesh(makeProfiles(4), 33, 5.0, nullptr, 100000);
    ASSERT_FALSE(mesh.empty());
    EXPECT_EQ(mesh.x.size(), mesh.y.size());
    EXPECT_EQ(mesh.x.size(), mesh.z.size());
    EXPECT_EQ(mesh.indices.size() % 3, 0u);
    for (const unsigned int index : mesh.indices) {
        EXPECT_LT(index, mesh.vertexCount());
    }
    // Four sections, both halves each, seventeen radii.
    EXPECT_EQ(mesh.vertexCount(), 8u * 17u);
}

TEST(MapMesh, DecimatesToTheVertexBudget) {
    const MapMesh mesh = buildMapMesh(makeProfiles(180), 33, 5.0, nullptr, 6000);
    ASSERT_FALSE(mesh.empty());
    EXPECT_LE(mesh.vertexCount(), 6000u);
    // A generous budget keeps every section.
    const MapMesh full = buildMapMesh(makeProfiles(180), 33, 5.0, nullptr, 1000000);
    EXPECT_GT(full.vertexCount(), mesh.vertexCount());
}

TEST(MapMesh, TheNominalSurfaceGivesTheDeviation) {
    std::vector<app::models::SurfaceData> profiles = makeProfiles(4, 0.02);
    for (auto& profile : profiles) {
        for (auto& point : profile.sagDataPoints) {
            point.sag = 0.02;
        }
    }
    app::models::SurfaceData nominal;
    nominal.semiDiameter = 5.0;
    nominal.sagDataPoints.resize(33);
    for (size_t i = 0; i < nominal.sagDataPoints.size(); ++i) {
        nominal.sagDataPoints[i].sag = 0.01;
    }
    const MapMesh mesh = buildMapMesh(profiles, 33, 5.0, &nominal, 100000);
    ASSERT_FALSE(mesh.empty());
    for (const float z : mesh.z) {
        EXPECT_NEAR(z, 0.01f, 1e-5f);
    }
}

TEST(MapMesh, TheVerticesFollowTheAngleAroundTheDisc) {
    // The sections contribute two halves each (the section and its opposite),
    // so the vertices must be laid out in angle order - otherwise the
    // triangles jump between the halves and fan out into spikes.
    const MapMesh mesh = buildMapMesh(makeProfiles(8), 33, 5.0, nullptr, 1000000);
    ASSERT_FALSE(mesh.empty());
    const size_t numRadii = 17;
    ASSERT_EQ(mesh.vertexCount() % numRadii, 0u);

    std::vector<double> angles;
    // The rim of every ring (the innermost vertex is the centre of the disc
    // and has no angle of its own).
    for (size_t v = numRadii - 1; v < mesh.vertexCount(); v += numRadii) {
        angles.push_back(std::atan2(static_cast<double>(mesh.y[v]), static_cast<double>(mesh.x[v])));
    }
    // Every angle step must be small: no vertex may jump across the disc.
    for (size_t i = 1; i < angles.size(); ++i) {
        double delta = angles[i] - angles[i - 1];
        while (delta < 0.0) delta += 2.0 * std::numbers::pi;
        EXPECT_LT(delta, std::numbers::pi / 2.0);
    }
}

TEST(MapMesh, TheCentreIsConnected) {
    // The innermost cells fan out from the shared centre vertex. Leaving them
    // out left a hole in the middle of the surface.
    const MapMesh mesh = buildMapMesh(makeProfiles(8), 33, 5.0, nullptr, 1000000);
    ASSERT_FALSE(mesh.empty());
    const size_t numRadii = 17;
    bool touchesCentre = false;
    for (const unsigned int index : mesh.indices) {
        if (index < numRadii) touchesCentre = true;
    }
    EXPECT_TRUE(touchesCentre);
}

TEST(MapMesh, TheRingIsClosed) {
    const MapMesh mesh = buildMapMesh(makeProfiles(8), 33, 5.0, nullptr, 1000000);
    ASSERT_FALSE(mesh.empty());
    // The last angle is connected back to the first one, so the surface has no
    // seam: the triangle set must cover the whole ring.
    const size_t numRadii = 17;
    const size_t rings = mesh.vertexCount() / numRadii;
    EXPECT_GT(rings, 2u);
    // Every triangle of the last ring reaches into the first ring.
    bool wraps = false;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const unsigned int a = mesh.indices[i];
        const unsigned int b = mesh.indices[i + 1];
        const unsigned int c = mesh.indices[i + 2];
        const bool touchesLast = a >= (rings - 1) * numRadii || b >= (rings - 1) * numRadii
                              || c >= (rings - 1) * numRadii;
        const bool touchesFirst = a < numRadii || b < numRadii || c < numRadii;
        if (touchesLast && touchesFirst) wraps = true;
    }
    EXPECT_TRUE(wraps);
}

} // namespace
} // namespace app::compute