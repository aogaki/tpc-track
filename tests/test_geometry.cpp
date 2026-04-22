#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cmath>
#include <stdexcept>
#include <string>

#include "tpctrack/geometry.hpp"

namespace {
constexpr double kPi = 3.14159265358979323846;
}

namespace {
const std::string kMiniGeoPath =
    std::string(TPCTRACK_TEST_DATA_DIR) + "/new_geometry_mini_eTPC.dat";
}

TEST_CASE("parseGeometry reads header constants from miniTPC geometry file") {
    auto geo = tpctrack::parseGeometry(kMiniGeoPath);

    CHECK(geo.header.angles_deg[0] == doctest::Approx(90.0));
    CHECK(geo.header.angles_deg[1] == doctest::Approx(-30.0));
    CHECK(geo.header.angles_deg[2] == doctest::Approx(30.0));
    CHECK(geo.header.diamond_size_mm == doctest::Approx(1.0));
    CHECK(geo.header.reference_point_x_mm == doctest::Approx(-53.250));
    CHECK(geo.header.reference_point_y_mm == doctest::Approx(-52.395));
    CHECK(geo.header.drift_velocity_cm_per_us == doctest::Approx(0.724));
    CHECK(geo.header.sampling_rate_mhz == doctest::Approx(25.0));
    CHECK(geo.header.trigger_delay_us == doctest::Approx(10.24));
    CHECK(geo.header.drift_cage_min_mm == doctest::Approx(-100.0));
    CHECK(geo.header.drift_cage_max_mm == doctest::Approx(100.0));
}

TEST_CASE("parseGeometry reads first U-plane strip metadata") {
    auto geo = tpctrack::parseGeometry(kMiniGeoPath);

    const auto& s = geo.stripInfo(tpctrack::Plane::U, 1);
    CHECK(s.strip_section == 0);
    CHECK(s.cobo == 0);
    CHECK(s.asad == 0);
    CHECK(s.aget == 2);
    CHECK(s.aget_ch == 62);
    CHECK(s.pad_pitch_offset == doctest::Approx(0.0));
    CHECK(s.strip_pitch_offset == doctest::Approx(-71.0));
    CHECK(s.length_in_pads == 56);
}

TEST_CASE("parseGeometry reads second U-plane strip metadata") {
    auto geo = tpctrack::parseGeometry(kMiniGeoPath);

    const auto& s = geo.stripInfo(tpctrack::Plane::U, 2);
    CHECK(s.aget == 1);
    CHECK(s.aget_ch == 0);
    CHECK(s.pad_pitch_offset == doctest::Approx(-0.5));
    CHECK(s.strip_pitch_offset == doctest::Approx(-70.0));
    CHECK(s.length_in_pads == 57);
}

TEST_CASE("stripInfo throws for unknown strip") {
    auto geo = tpctrack::parseGeometry(kMiniGeoPath);
    CHECK_THROWS_AS(geo.stripInfo(tpctrack::Plane::U, 99999),
                    std::out_of_range);
}

TEST_CASE("stripAxisDirection returns unit vector at the U/V/W angle") {
    auto geo = tpctrack::parseGeometry(kMiniGeoPath);

    auto [ux, uy] = geo.stripAxisDirection(tpctrack::Plane::U);
    CHECK(ux == doctest::Approx(0.0).epsilon(1e-9));
    CHECK(uy == doctest::Approx(1.0).epsilon(1e-9));

    auto [vx, vy] = geo.stripAxisDirection(tpctrack::Plane::V);
    CHECK(vx == doctest::Approx(std::cos(-30.0 * kPi / 180.0)).epsilon(1e-9));
    CHECK(vy == doctest::Approx(std::sin(-30.0 * kPi / 180.0)).epsilon(1e-9));

    auto [wx, wy] = geo.stripAxisDirection(tpctrack::Plane::W);
    CHECK(wx == doctest::Approx(std::cos(30.0 * kPi / 180.0)).epsilon(1e-9));
    CHECK(wy == doctest::Approx(std::sin(30.0 * kPi / 180.0)).epsilon(1e-9));
}

TEST_CASE("parseGeometry throws when the file cannot be opened") {
    CHECK_THROWS(tpctrack::parseGeometry("/nonexistent/path.dat"));
}
