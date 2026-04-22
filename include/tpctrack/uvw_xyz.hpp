#pragma once

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

// Combine 3-plane hits into 3D points. Minimal v1: for each time_bin where
// all three planes have exactly one hit, solve for (x, y) from U+V and take
// z from the time_bin. charge is the 3-plane mean. Other time_bins are skipped.
std::vector<Point3D> hitsToPoints(const Geometry& geo,
                                  const std::vector<Hit>& hits);

}  // namespace tpctrack
