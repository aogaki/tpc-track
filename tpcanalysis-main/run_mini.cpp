// run_mini.cpp — single-argument production macro for the 3D pipeline.
//
// Reads one miniTPC .root file, runs the full loadData → convertUVW_mini
// → cleanUVW → extractHits → hitsToPoints chain for every event, and
// writes a single CSV with columns (event_id, x_mm, y_mm, z_mm, charge).
//
// Usage (from tpcanalysis-main/):
//   root -b -q 'run_mini.cpp("<root>")'
//
// Output:
//   <root> の隣に <rootname>_points.csv を作る

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "TROOT.h"
#include "TString.h"
#include "TSystem.h"

// Header search path and libMyLib linking are set by rootlogon.C in this
// directory, which ROOT auto-loads on startup. If you're invoking root
// from somewhere else, source that logon manually or set gSystem yourself.

// Load the pre-built GET dictionary from tpcanalysis-main/dict/build/.
// No extension: gSystem picks .dylib on macOS and .so on Linux.
//
// Build it once with:
//   (cd dict && cmake -B build -S . && cmake --build build)
R__LOAD_LIBRARY(dict/build/libMyLib)

// tpcanalysis-main upstream pipeline.
#include "include/ErrorCodesMap.hpp"
#include "src/cleanUVW.cpp"
#include "src/convertUVW_mini.cpp"
#include "src/loadData.cpp"
#include "include/generalDataStorage.hpp"

// tpctrack new pipeline. Header search path set via the pragma above.
#include "../include/tpctrack/geometry.hpp"
#include "../include/tpctrack/hit_extraction.hpp"
#include "../include/tpctrack/uvw_xyz.hpp"
#include "../src/geometry.cpp"
#include "../src/hit_extraction.cpp"
#include "../src/uvw_xyz.cpp"

namespace {

// Fixed pipeline defaults — no user-facing flags (KISS).
constexpr bool kNorm = true;
constexpr bool kClean = true;
constexpr double kHitThreshold = 30.0;   // ADC above cleaned baseline
// 3-plane charges are expected to agree within ~2× for genuine ionization
// hits. This kills the huge cartesian-product ghost explosion from
// `hitsToPoints` down to a Plotly-manageable count.
constexpr double kMaxChargeRatio = 2.0;

std::string stripExtension(const std::string& path) {
    auto dot = path.find_last_of('.');
    return (dot == std::string::npos) ? path : path.substr(0, dot);
}

}  // namespace

void run_mini(TString rootFile) {
    // --- Load geometry ----------------------------------------------------
    const std::string geo_path = "utils/new_geometry_mini_eTPC.dat";
    auto geo = tpctrack::parseGeometry(geo_path);

    // --- Open .root -------------------------------------------------------
    loadData ld(rootFile, "tree");
    if (ld.openFile() != 0) {
        std::cerr << "Cannot open " << rootFile << "\n";
        return;
    }
    ld.readData();
    const auto n_entries = ld.returnNEntries();
    std::cout << "Entries: " << n_entries << "\n";

    // --- Prepare converters / cleaner once --------------------------------
    convertUVW_mini conv;
    if (conv.openSpecFile() != 0) {
        std::cerr << "convertUVW_mini::openSpecFile failed\n";
        return;
    }
    cleanUVW clean;

    // --- Open output CSV --------------------------------------------------
    const std::string out_csv =
        stripExtension(std::string(rootFile.Data())) + "_points.csv";
    std::ofstream csv(out_csv);
    if (!csv) {
        std::cerr << "Cannot write " << out_csv << "\n";
        return;
    }
    csv << "event_id,x_mm,y_mm,z_mm,charge\n";
    csv.precision(6);

    // --- Loop over all events --------------------------------------------
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

        // event_id is the same for every strip in one decoded entry.
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
              << " hits) over " << n_entries << " events to " << out_csv
              << "\n";
}
