#pragma once

#include <array>
#include <map>
#include <string>
#include <utility>

namespace tpctrack {

enum class Plane { U = 0, V = 1, W = 2 };

struct GeometryHeader {
    std::array<double, 3> angles_deg{};
    double diamond_size_mm{};
    double reference_point_x_mm{};
    double reference_point_y_mm{};
    double drift_velocity_cm_per_us{};
    double sampling_rate_mhz{};
    double trigger_delay_us{};
    double drift_cage_min_mm{};
    double drift_cage_max_mm{};
};

struct StripInfo {
    int strip_section{};
    int cobo{};
    int asad{};
    int aget{};
    int aget_ch{};
    double pad_pitch_offset{};
    double strip_pitch_offset{};
    int length_in_pads{};
};

class Geometry {
  public:
    GeometryHeader header;

    const StripInfo& stripInfo(Plane p, int strip_nr) const;
    std::pair<double, double> stripAxisDirection(Plane p) const;

    void addStrip(Plane p, int strip_nr, StripInfo info);

  private:
    std::map<std::pair<int, int>, StripInfo> strips_;
};

Geometry parseGeometry(const std::string& path);

}  // namespace tpctrack
