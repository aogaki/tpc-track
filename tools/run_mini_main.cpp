// run_mini — standalone executable that turns one miniTPC .root file into a
// 3D point-cloud CSV.
//
// Usage (from project root):
//   ./build/run_mini path/to/input.root [path/to/output.csv]
//
// If the output path is omitted, the CSV is written next to the input as
// <input-stem>_points.csv. The binary expects to find utils/ in the current
// working directory, so it must be launched from the project root where
// utils/new_geometry_mini_eTPC.dat and utils/ch_norm_ratios_mini.csv live.
//
// The pipeline is: loadData -> convertUVW_mini -> cleanUVW -> extractHits ->
// hitsToPoints. Knobs are fixed at build time (no command-line flags).

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "TString.h"

// Upstream miniTPC reader
#include "cleanUVW.hpp"
#include "convertUVW_mini.hpp"
#include "loadData.hpp"

// tpctrack pipeline (ROOT-free)
#include "tpctrack/geometry.hpp"
#include "tpctrack/hit_extraction.hpp"
#include "tpctrack/uvw_xyz.hpp"

namespace {

// Fixed pipeline defaults — no user-facing flags (KISS).
constexpr bool kNorm = true;
constexpr bool kClean = true;
constexpr double kHitThreshold = 30.0;   // ADC above cleaned baseline
// 3-plane charges agree within ~2× for genuine ionization hits; this kills
// most of the cartesian-product ghost explosion from hitsToPoints.
constexpr double kMaxChargeRatio = 2.0;

std::string stripExtension(const std::string& path) {
    const auto dot = path.find_last_of('.');
    return (dot == std::string::npos) ? path : path.substr(0, dot);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <input.root> [output.csv]\n";
        return 1;
    }
    const std::string input = argv[1];
    const std::string output =
        (argc == 3) ? std::string(argv[2]) : (stripExtension(input) + "_points.csv");

    // --- Load geometry (relative to cwd) ---------------------------------
    auto geo = tpctrack::parseGeometry("utils/new_geometry_mini_eTPC.dat");

    // --- Open .root ------------------------------------------------------
    loadData ld(TString(input.c_str()), "tree");
    if (ld.openFile() != 0) {
        std::cerr << "Cannot open " << input << "\n";
        return 2;
    }
    ld.readData();
    const auto n_entries = ld.returnNEntries();
    std::cout << "Entries: " << n_entries << "\n";

    // --- Prepare converter / cleaner once --------------------------------
    convertUVW_mini conv;
    if (conv.openSpecFile() != 0) {
        std::cerr << "convertUVW_mini::openSpecFile failed (run from project root?)\n";
        return 3;
    }
    cleanUVW clean;

    // --- Open CSV --------------------------------------------------------
    std::ofstream csv(output);
    if (!csv) {
        std::cerr << "Cannot write " << output << "\n";
        return 4;
    }
    csv << "event_id,x_mm,y_mm,z_mm,charge\n";
    csv.precision(6);

    // --- Loop over every event ------------------------------------------
    std::size_t total_hits = 0;
    std::size_t total_points = 0;
    for (int entry = 0; entry < n_entries; ++entry) {
        if (ld.decodeData(entry, /*remove_fpn=*/true) != 0) continue;
        conv.setRawData(ld.returnRawData());
        if (conv.makeConversion(kNorm, /*verbose=*/false) != 0) continue;
        auto uvw = conv.returnDataUVW();
        if (uvw.empty()) continue;

        if (kClean) {
            clean.setUVWData(uvw);
            clean.substractBl<cleanUVW::miniPlaneInfoU>(true);
            clean.substractBl<cleanUVW::miniPlaneInfoV>(true);
            clean.substractBl<cleanUVW::miniPlaneInfoW>(true);
            uvw = clean.returnDataUVW();
        }

        const uint32_t event_id = uvw.front().event_id;

        std::vector<tpctrack::StripSignal> signals;
        signals.reserve(uvw.size());
        for (const auto& d : uvw) {
            signals.push_back({d.plane_val, d.strip_nr, d.signal_val});
        }

        auto hits = tpctrack::extractHits(signals, kHitThreshold);
        total_hits += hits.size();
        auto pts = tpctrack::hitsToPoints(geo, hits, kMaxChargeRatio);
        total_points += pts.size();

        for (const auto& p : pts) {
            csv << event_id << "," << p.x_mm << "," << p.y_mm << "," << p.z_mm
                << "," << p.charge << "\n";
        }

        if (entry % 20 == 0 || entry == n_entries - 1) {
            std::cout << "entry " << entry << " / " << n_entries
                      << "  hits=" << hits.size() << "  points=" << pts.size()
                      << "\n";
        }
    }

    csv.close();
    std::cout << "\nWrote " << total_points << " points (from " << total_hits
              << " hits) over " << n_entries << " events to " << output << "\n";
    return 0;
}
