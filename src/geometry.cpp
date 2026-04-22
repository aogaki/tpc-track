#include "tpctrack/geometry.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace tpctrack {

namespace {

constexpr double kDegToRad = 3.14159265358979323846 / 180.0;

std::string rtrim(std::string s) {
    while (!s.empty() &&
           std::isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    return s;
}

Plane planeFromChar(char c) {
    switch (c) {
        case 'U': return Plane::U;
        case 'V': return Plane::V;
        case 'W': return Plane::W;
        default:
            throw std::runtime_error(std::string("Unknown plane char: ") + c);
    }
}

void parseHeaderLine(const std::string& line, GeometryHeader& h) {
    auto colon = line.find(':');
    if (colon == std::string::npos) return;
    const std::string key = rtrim(line.substr(0, colon));
    std::istringstream iss(line.substr(colon + 1));

    if (key == "ANGLES") {
        iss >> h.angles_deg[0] >> h.angles_deg[1] >> h.angles_deg[2];
    } else if (key == "DIAMOND SIZE") {
        iss >> h.diamond_size_mm;
    } else if (key == "REFERENCE POINT") {
        iss >> h.reference_point_x_mm >> h.reference_point_y_mm;
    } else if (key == "DRIFT VELOCITY") {
        iss >> h.drift_velocity_cm_per_us;
    } else if (key == "SAMPLING RATE") {
        iss >> h.sampling_rate_mhz;
    } else if (key == "TRIGGER DELAY") {
        iss >> h.trigger_delay_us;
    } else if (key == "DRIFT CAGE ACCEPTANCE") {
        iss >> h.drift_cage_min_mm >> h.drift_cage_max_mm;
    }
}

}  // namespace

void Geometry::addStrip(Plane p, int strip_nr, StripInfo info) {
    strips_[{static_cast<int>(p), strip_nr}] = info;
}

const StripInfo& Geometry::stripInfo(Plane p, int strip_nr) const {
    auto it = strips_.find({static_cast<int>(p), strip_nr});
    if (it == strips_.end()) {
        throw std::out_of_range(
            "No strip info for plane=" + std::to_string(static_cast<int>(p)) +
            " strip=" + std::to_string(strip_nr));
    }
    return it->second;
}

std::pair<double, double> Geometry::stripAxisDirection(Plane p) const {
    const double theta = header.angles_deg[static_cast<int>(p)] * kDegToRad;
    return {std::cos(theta), std::sin(theta)};
}

Geometry parseGeometry(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Cannot open geometry file: " + path);
    }

    Geometry geo;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line[0] == 'U' || line[0] == 'V' || line[0] == 'W') {
            std::istringstream iss(line);
            char plane_char = 0;
            int strip_nr = 0;
            StripInfo s;
            iss >> plane_char >> s.strip_section >> strip_nr >> s.cobo >>
                s.asad >> s.aget >> s.aget_ch >> s.pad_pitch_offset >>
                s.strip_pitch_offset >> s.length_in_pads;
            if (!iss) continue;
            geo.addStrip(planeFromChar(plane_char), strip_nr, s);
        } else {
            parseHeaderLine(line, geo.header);
        }
    }
    return geo;
}

}  // namespace tpctrack
