#include <gtest/gtest.h>

#include <limits>
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

} // namespace
} // namespace app::compute