#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cmath>
#include <string>

#include "tpctrack/uvw_xyz.hpp"

namespace {
constexpr double kPi = 3.14159265358979323846;
const std::string kMiniGeoPath =
    std::string(TPCTRACK_TEST_DATA_DIR) + "/new_geometry_mini_eTPC.dat";

tpctrack::Geometry loadGeo() { return tpctrack::parseGeometry(kMiniGeoPath); }
}  // namespace

TEST_CASE("projectXY for U plane (theta=90deg) returns the y coordinate") {
    auto geo = loadGeo();
    CHECK(tpctrack::projectXY(geo, tpctrack::Plane::U, 3.0, 5.0) ==
          doctest::Approx(5.0));
}

TEST_CASE("projectXY for V plane (theta=-30deg) matches cos/sin") {
    auto geo = loadGeo();
    double got = tpctrack::projectXY(geo, tpctrack::Plane::V, 1.0, 0.0);
    CHECK(got == doctest::Approx(std::cos(-30.0 * kPi / 180.0)).epsilon(1e-9));
}

TEST_CASE("stripCoordMm is monotonic in strip_pitch_offset") {
    auto geo = loadGeo();
    double u1 = tpctrack::stripCoordMm(geo, tpctrack::Plane::U, 1);
    double u2 = tpctrack::stripCoordMm(geo, tpctrack::Plane::U, 2);
    double u3 = tpctrack::stripCoordMm(geo, tpctrack::Plane::U, 3);
    // Strip offsets in the mini file are -71, -70, -69, i.e. increasing by 1
    // per strip; stripCoordMm should therefore also be monotonically increasing.
    CHECK(u1 < u2);
    CHECK(u2 < u3);
    // The increment should equal the strip pitch (same on both steps).
    CHECK((u2 - u1) == doctest::Approx(u3 - u2).epsilon(1e-9));
}

TEST_CASE("xyFromTwoCoords is the inverse of projectXY (round-trip)") {
    auto geo = loadGeo();
    const double x = 3.5, y = -2.7;
    double u = tpctrack::projectXY(geo, tpctrack::Plane::U, x, y);
    double v = tpctrack::projectXY(geo, tpctrack::Plane::V, x, y);
    auto [rx, ry] = tpctrack::xyFromTwoCoords(geo, tpctrack::Plane::U, u,
                                              tpctrack::Plane::V, v);
    CHECK(rx == doctest::Approx(x).epsilon(1e-9));
    CHECK(ry == doctest::Approx(y).epsilon(1e-9));
}

TEST_CASE("zFromTimeBin is 0 at the trigger bin (256)") {
    auto geo = loadGeo();
    // trigger_delay 10.24 us * 25 MHz = 256 bins exactly.
    CHECK(tpctrack::zFromTimeBin(geo, 256) ==
          doctest::Approx(0.0).epsilon(1e-9));
}

TEST_CASE("zFromTimeBin has slope v_drift * bin_duration") {
    auto geo = loadGeo();
    const double expected_mm_per_bin =
        (1.0 / 25.0) * 0.724 * 10.0;  // (1/MHz) us/bin * cm/us * 10 mm/cm
    double dz =
        tpctrack::zFromTimeBin(geo, 257) - tpctrack::zFromTimeBin(geo, 256);
    CHECK(dz == doctest::Approx(expected_mm_per_bin).epsilon(1e-9));
}

TEST_CASE("hitsToPoints emits one Point3D per time_bin with all 3 planes") {
    auto geo = loadGeo();
    std::vector<tpctrack::Hit> hits{
        {0, 1, 300, 100.0},
        {1, 1, 300, 120.0},
        {2, 1, 300, 110.0},
    };
    auto pts = tpctrack::hitsToPoints(geo, hits);
    REQUIRE(pts.size() == 1);
    CHECK(pts[0].z_mm ==
          doctest::Approx(tpctrack::zFromTimeBin(geo, 300)).epsilon(1e-9));
    CHECK(pts[0].charge == doctest::Approx((100.0 + 120.0 + 110.0) / 3.0));
}

TEST_CASE("hitsToPoints skips time_bins missing any of the three planes") {
    auto geo = loadGeo();
    std::vector<tpctrack::Hit> hits{
        {0, 1, 300, 100.0},  // U only
        {1, 1, 301, 120.0},  // V only
        {2, 1, 302, 110.0},  // W only
    };
    auto pts = tpctrack::hitsToPoints(geo, hits);
    CHECK(pts.empty());
}

TEST_CASE("hitsToPoints skips time_bins with duplicate hits on a plane") {
    auto geo = loadGeo();
    std::vector<tpctrack::Hit> hits{
        {0, 1, 300, 100.0},
        {0, 2, 300, 110.0},  // duplicate U hit at same time_bin
        {1, 1, 300, 120.0},
        {2, 1, 300, 110.0},
    };
    auto pts = tpctrack::hitsToPoints(geo, hits);
    CHECK(pts.empty());
}
