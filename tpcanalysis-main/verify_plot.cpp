// verify_plot.cpp — M4 checkpoint macro.
//
// Produces UVW PNGs under the same conditions as
//   runmacro_mini({"-convert","-norm0","-clean0","-zip0", ...})
// so they can be diffed pixel-by-pixel (scripts/compare_png.py), and
// additionally emits a time-integrated XY PNG via the tpctrack M1–M3
// pipeline for visual geometry validation.
//
// Usage (from tpcanalysis-main/):
//   root -b -q 'verify_plot.cpp+("../raw_run_files/CoBo_....root", 0)'
//
// Output:
//   ../verification/new/<rootname>/<event>_{u,v,w,xy}.png

#include <iostream>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TH2D.h"
#include "TROOT.h"
#include "TString.h"
#include "TStyle.h"

// Header search path and libMyLib linking come from tpcanalysis-main/rootlogon.C.

// Load the pre-built GET dictionary. See run_mini.cpp for build notes.
// No extension: gSystem picks .dylib on macOS and .so on Linux.
R__LOAD_LIBRARY(dict/build/libMyLib)

// tpcanalysis-main upstream pipeline (loadData + convertUVW_mini, NO cleanUVW).
#include "include/ErrorCodesMap.hpp"
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

std::string rootBasename(const std::string& path) {
    auto slash = path.find_last_of('/');
    std::string name =
        (slash == std::string::npos) ? path : path.substr(slash + 1);
    auto dot = name.find_last_of('.');
    if (dot != std::string::npos) name = name.substr(0, dot);
    return name;
}

}  // namespace

void verify_plot(TString rootFile, int event_nr = 0) {
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);

    // --- 1. Read one event from the .root file -----------------------------
    loadData ld(rootFile, "tree");
    if (ld.openFile() != 0) {
        std::cerr << "Cannot open " << rootFile << "\n";
        return;
    }
    ld.readData();
    const auto n_entries = ld.returnNEntries();
    std::cout << "File has " << n_entries << " entries\n";
    if (event_nr >= n_entries) {
        std::cerr << "event " << event_nr << " out of range\n";
        return;
    }
    ld.decodeData(event_nr, /*remove_fpn=*/true);  // same default as runmacro_mini
    auto raw = ld.returnRawData();

    // --- 2. rawData → dataUVW (norm=false, matching -norm0) ---------------
    convertUVW_mini conv;
    if (conv.openSpecFile() != 0) {
        std::cerr << "Cannot open utils/new_geometry_mini_eTPC.dat — "
                     "run this macro from the tpcanalysis-main/ directory.\n";
        return;
    }
    conv.setRawData(raw);
    conv.makeConversion(/*opt_norm=*/false, /*opt_verbose=*/false);
    auto uvw = conv.returnDataUVW();

    // --- 3. Output directory -----------------------------------------------
    const std::string outBase =
        "../verification/new/" + rootBasename(std::string(rootFile.Data()));
    gROOT->ProcessLine((".! mkdir -p " + outBase).c_str());

    // --- 4. UVW PNGs (mirrors drawUVWimage_mini layout byte-for-byte) ------
    TCanvas canv("uvw", "UVW hists", 800, 600);
    canv.SetFrameLineColor(0);
    canv.SetFrameLineWidth(0);
    canv.SetBottomMargin(0);
    canv.SetTopMargin(0);
    canv.SetLeftMargin(0);
    canv.SetRightMargin(0);

    auto* hu = new TH2D(Form("u_hists_%d", event_nr), "", 512, 1, 512, 72, 1, 72);
    auto* hv = new TH2D(Form("v_hists_%d", event_nr), "", 512, 1, 512, 92, 1, 92);
    auto* hw = new TH2D(Form("w_hists_%d", event_nr), "", 512, 1, 512, 92, 1, 92);
    for (TH2D* h : {hu, hv, hw}) {
        h->SetMinimum(-1);
        h->SetMaximum(1000);
        h->SetTickLength(0);
        h->GetYaxis()->SetTickLength(0);
        h->SetLabelSize(0);
        h->GetYaxis()->SetLabelSize(0);
    }

    for (const auto& d : uvw) {
        TH2D* target = (d.plane_val == 0) ? hu
                       : (d.plane_val == 1) ? hv
                       : (d.plane_val == 2) ? hw
                                            : nullptr;
        if (!target) continue;
        int bin = 0;
        for (double val : d.signal_val) {
            target->SetBinContent(++bin, d.strip_nr, val);
        }
    }

    const std::string stem = outBase + "/" + std::to_string(event_nr);

    hu->Draw("COL");
    canv.SetWindowSize(512, 3 * 72);
    canv.Update();
    canv.Print((stem + "_u.png").c_str());

    hv->Draw("COL");
    canv.SetWindowSize(512, 3 * 92);
    canv.Update();
    canv.Print((stem + "_v.png").c_str());

    hw->Draw("COL");
    canv.SetWindowSize(512, 3 * 92);
    canv.Update();
    canv.Print((stem + "_w.png").c_str());

    // --- 5. tpctrack pipeline → XY PNG -------------------------------------
    auto geo = tpctrack::parseGeometry("utils/new_geometry_mini_eTPC.dat");

    std::vector<tpctrack::StripSignal> signals;
    signals.reserve(uvw.size());
    for (const auto& d : uvw) {
        signals.push_back({d.plane_val, d.strip_nr, d.signal_val});
    }

    // With norm=false, clean=false, baseline is ~100 ADC. Pick a threshold
    // clearly above it so noise is rejected but signal hits survive.
    const double threshold = 200.0;
    auto hits = tpctrack::extractHits(signals, threshold);
    auto pts = tpctrack::hitsToPoints(geo, hits);

    std::cout << "hits=" << hits.size() << "  3D points=" << pts.size() << "\n";

    // XY visualization: rasterize each strip line weighted by hit charge.
    // Every hit lives on a line in the (x, y) plane determined by its strip's
    // u-coordinate and the plane's angle. Intersections of the three line
    // families (U/V/W) indicate where the actual interactions happened.
    // This validates the geometry (angles, reference point, pitch) even when
    // hitsToPoints' strict 3-plane matching yields no points.
    TH2D hxy(Form("xy_%d", event_nr), ";x [mm];y [mm]",
             300, -150, 150, 300, -150, 150);
    const double t_min = -100.0, t_max = 100.0, t_step = 0.5;
    for (const auto& h : hits) {
        const auto plane = static_cast<tpctrack::Plane>(h.plane);
        const double u = tpctrack::stripCoordMm(geo, plane, h.strip);
        auto [ax, ay] = geo.stripAxisDirection(plane);
        // Strip line: (x, y) = u * (ax, ay) + t * (-ay, ax)
        for (double t = t_min; t <= t_max; t += t_step) {
            const double x = u * ax - t * ay;
            const double y = u * ay + t * ax;
            hxy.Fill(x, y, h.charge);
        }
    }

    TCanvas cxy("xy", "XY projection", 700, 600);
    cxy.SetRightMargin(0.14);
    hxy.Draw("COLZ");
    cxy.Update();
    cxy.Print((stem + "_xy.png").c_str());

    std::cout << "Wrote PNGs to " << outBase << "/\n";
}
