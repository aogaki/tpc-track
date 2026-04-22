#pragma once

#include <limits>
#include <utility>
#include <vector>

#include "tpctrack/geometry.hpp"
#include "tpctrack/hit.hpp"

namespace tpctrack {

struct Point3D {
    double x_mm;
    double y_mm;
    double z_mm;
    double charge;
};

// Project an (x, y) point onto plane p's u axis.
double projectXY(const Geometry& geo, Plane p, double x_mm, double y_mm);

// Physical u-coordinate (mm) of the center line of the given strip.
double stripCoordMm(const Geometry& geo, Plane p, int strip_nr);

// Solve for (x, y) given the u-coordinate on two different planes.
std::pair<double, double> xyFromTwoCoords(const Geometry& geo,
                                          Plane plane_a, double u_a,
                                          Plane plane_b, double u_b);

// time_bin (0..511) → z in mm, taking TRIGGER DELAY as z=0.
double zFromTimeBin(const Geometry& geo, int time_bin);

// Combine 3-plane hits into 3D points. For each time_bin where all three
// planes have at least one hit, emit the cartesian product of (U × V × W).
// (x, y) is solved from U+V; z from the time_bin; charge is the 3-plane mean.
//
// max_charge_ratio: if finite, drop combinations where
//     max(c_u, c_v, c_w) / min(c_u, c_v, c_w) > max_charge_ratio
// i.e., 3-plane charges must agree within that factor. A typical ionization
// hit has roughly equal charges on all 3 planes, so this filter kills most
// ghost combinations. Default (infinity) disables filtering — all combos kept.
std::vector<Point3D> hitsToPoints(
    const Geometry& geo, const std::vector<Hit>& hits,
    double max_charge_ratio = std::numeric_limits<double>::infinity());

}  // namespace tpctrack
