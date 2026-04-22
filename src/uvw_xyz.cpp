#include "tpctrack/uvw_xyz.hpp"

#include <array>
#include <cmath>
#include <map>
#include <stdexcept>

namespace tpctrack {

namespace {

double stripPitchMm(const Geometry& geo) {
    // Hex-packed diamond pads: perpendicular distance between adjacent strip
    // centers is DIAMOND_SIZE * sqrt(3)/2. This is the working assumption for
    // M3; the M4 PNG comparison will validate or force adjustment.
    return geo.header.diamond_size_mm * std::sqrt(3.0) / 2.0;
}

double mmPerBin(const Geometry& geo) {
    return (1.0 / geo.header.sampling_rate_mhz) *
           geo.header.drift_velocity_cm_per_us * 10.0;
}

double triggerBin(const Geometry& geo) {
    return geo.header.trigger_delay_us * geo.header.sampling_rate_mhz;
}

}  // namespace

double projectXY(const Geometry& geo, Plane p, double x_mm, double y_mm) {
    auto [ax, ay] = geo.stripAxisDirection(p);
    return x_mm * ax + y_mm * ay;
}

double stripCoordMm(const Geometry& geo, Plane p, int strip_nr) {
    const auto& info = geo.stripInfo(p, strip_nr);
    const double ref_u = projectXY(geo, p,
                                   geo.header.reference_point_x_mm,
                                   geo.header.reference_point_y_mm);
    return ref_u + info.strip_pitch_offset * stripPitchMm(geo);
}

std::pair<double, double> xyFromTwoCoords(const Geometry& geo,
                                          Plane pa, double ua,
                                          Plane pb, double ub) {
    auto [ax, ay] = geo.stripAxisDirection(pa);
    auto [bx, by] = geo.stripAxisDirection(pb);
    const double det = ax * by - ay * bx;
    if (std::abs(det) < 1e-12) {
        throw std::runtime_error("xyFromTwoCoords: planes are parallel");
    }
    const double x = (ua * by - ub * ay) / det;
    const double y = (ax * ub - bx * ua) / det;
    return {x, y};
}

double zFromTimeBin(const Geometry& geo, int time_bin) {
    return (static_cast<double>(time_bin) - triggerBin(geo)) * mmPerBin(geo);
}

std::vector<Point3D> hitsToPoints(const Geometry& geo,
                                  const std::vector<Hit>& hits) {
    std::map<int, std::array<std::vector<const Hit*>, 3>> by_bin;
    for (const auto& h : hits) {
        if (h.plane < 0 || h.plane > 2) continue;
        by_bin[h.time_bin][h.plane].push_back(&h);
    }

    std::vector<Point3D> pts;
    for (const auto& [bin, per_plane] : by_bin) {
        if (per_plane[0].size() != 1 || per_plane[1].size() != 1 ||
            per_plane[2].size() != 1) {
            continue;
        }
        const Hit& hu = *per_plane[0][0];
        const Hit& hv = *per_plane[1][0];
        const Hit& hw = *per_plane[2][0];

        const double u = stripCoordMm(geo, Plane::U, hu.strip);
        const double v = stripCoordMm(geo, Plane::V, hv.strip);
        auto [x, y] = xyFromTwoCoords(geo, Plane::U, u, Plane::V, v);

        Point3D p;
        p.x_mm = x;
        p.y_mm = y;
        p.z_mm = zFromTimeBin(geo, bin);
        p.charge = (hu.charge + hv.charge + hw.charge) / 3.0;
        pts.push_back(p);
    }
    return pts;
}

}  // namespace tpctrack
