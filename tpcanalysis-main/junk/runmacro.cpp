#include <cstdio>
#include <glob.h>
#include <iostream>
#include <limits>
#include <map>
#include <random>

#include "TString.h"
#include "include/ErrorCodesMap.hpp"
#include "src/cleanUVW.cpp"
#include "src/convertHitData.cpp"
#include "src/convertUVW.cpp"
#include "src/convertXYZ.cpp"
#include "src/filterEventsXY.cpp"
#include "src/loadData.cpp"

#include "include/generalDataStorage.hpp"

#include "TCanvas.h"
#include "TColor.h"
#include "TError.h"
#include "TF1.h"
#include "TH2D.h"
#include "TMarker.h"
#include "TROOT.h"
#include "TSpectrum.h"
#include "TStyle.h"
#include "TSystem.h"

#include "dict/src/GDataChannel.cpp"
#include "dict/src/GDataFrame.cpp"
#include "dict/src/GDataSample.cpp"
#include "dict/src/GFrameHeader.cpp"

/**
 * @brief Allows the viewing of the raw data from the given file.
 *
 * @param fileName The file to viewed.
 * @param norm_opt The normalization option. 1 if you want the channels to be
 * normalized, else 0.
 */
void view_raw_data(TString fileName, int norm_opt = 1) {

    int entry_nr = -1;
    int max_entries;

    auto goodFile = fileName;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    max_entries = good_data.returnNEntries();

    convertUVW loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    auto loc_canv =
        new TCanvas("View entries canvas", "View entries", 1500, 1000);
    auto loc_pad = new TPad("pad name", "pad title", 0, 0, 1, 1);
    loc_pad->Divide(3, 1);
    loc_pad->Draw();

    TH2D *u_hists = nullptr;
    TH2D *v_hists = nullptr;
    TH2D *w_hists = nullptr;

    uint32_t loop_nr = 0;

    while (1) {

        generalDataStorage data_container;

        std::cout
            << "Press 'e' to exit, 'd' for next and 'a' for previous and i "
               "to input the entry number. Loop "
            << loop_nr << std::endl;

        loop_nr++;

        char key_val = getchar();

        // Clear the input buffer
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (key_val == 'e') {

            break;

        } else if (key_val == 'd') {

            if (entry_nr < max_entries) {
                entry_nr++;
            }

            std::cout << "Entry number is " << entry_nr << "\n\n" << std::endl;

        } else if (key_val == 'a') {

            if (entry_nr > 0) {
                entry_nr--;
            }

            std::cout << "Entry number is " << entry_nr << "\n\n" << std::endl;

        } else if (key_val == 'i') {

            std::cout << "Please enter the entry number.\n";

            std::cin >> entry_nr;

            if (entry_nr < 0 || entry_nr > (max_entries - 1))
                entry_nr = 0;

            std::cout << "Entry number is " << entry_nr << "\n\n" << std::endl;
        }

        if (entry_nr < 0) {
            entry_nr = 0;
        }

        err = good_data.decodeData(entry_nr);
        if (err != 0) {
            std::cout << "Error decode data code " << err << std::endl;
        }

        data_container.root_raw_data = good_data.returnRawData();

        std::cout << "RAW data size is " << data_container.root_raw_data.size()
                  << std::endl;

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion();
        if (err != 0)
            std::cout << "Make conversion error code " << err << std::endl;

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        std::cout << "UVW data size is " << data_container.uvw_data.size()
                  << std::endl;

        if (u_hists != nullptr)
            u_hists->Reset();

        if (v_hists != nullptr)
            v_hists->Reset();

        if (w_hists != nullptr)
            w_hists->Reset();

        u_hists = new TH2D(Form("u_hists_%d", entry_nr),
                           Form("Histogram for U entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        v_hists = new TH2D(Form("v_hists_%d", entry_nr),
                           Form("Histogram for V entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        w_hists = new TH2D(Form("w_hists_%d", entry_nr),
                           Form("Histogram for W entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);

        int bin = 0;

        for (const auto &iter : data_container.uvw_data) {

            bin = 0;

            if (iter.plane_val == 0) {

                for (auto sig_iter : iter.signal_val) {

                    u_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 1) {

                for (auto sig_iter : iter.signal_val) {

                    v_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 2) {

                for (auto sig_iter : iter.signal_val) {

                    w_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }
        }

        loc_pad->cd(1);
        u_hists->Draw("COLZ");
        loc_pad->cd(2);
        v_hists->Draw("COLZ");
        loc_pad->cd(3);
        w_hists->Draw("COLZ");

        loc_canv->Update();
    }

    loc_canv->WaitPrimitive();
}

/**
 * @brief Create a pdf file containing the entries from the .root file.
 *
 * @param source_file The source .root file.
 * @param destination_file The name and loation of the pdf file to be created.
 * @param read_entries The number of entries read.
 * @param norm_opt The normalization option. 1 if you want the channels to be
 * normalized, else 0.
 */
void create_entries_pdf(TString source_file, TString destination_file,
                        int read_entries) {

    int entry_nr = 0;
    int max_entries;

    auto goodFile = source_file;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    max_entries = good_data.returnNEntries();

    convertUVW loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    cleanUVW loc_clean_uvw;

    convertHitData loc_convert_hit;

    convertXYZ loc_conv_xyz;

    gROOT->SetBatch(kTRUE);

    auto loc_canv = new TCanvas("entries.pdf", "PDF", 1500, 1000);
    auto loc_pad = new TPad("pad name", "pad title", 0, 0, 1, 1);
    loc_pad->Divide(3, 3);
    loc_pad->Draw();

    TH2D *u_hists = nullptr;
    TH2D *v_hists = nullptr;
    TH2D *w_hists = nullptr;

    TGraph *u_graph = nullptr;
    TGraph *v_graph = nullptr;
    TGraph *w_graph = nullptr;

    TGraph *p_graph = nullptr;
    TGraph2D *p_graph3d = nullptr;

    loc_canv->Print(destination_file + "[");
    gErrorIgnoreLevel = kWarning;

    // only read the number of entries wanted
    max_entries = std::min(max_entries, read_entries);

    while (entry_nr < max_entries) {

        generalDataStorage data_container;

        std::cout << "\n\n\nNow at entry: " << entry_nr << " of " << max_entries
                  << " for " << destination_file << "\n";

        err = good_data.decodeData(entry_nr);
        if (err != 0) {
            std::cout << "Error decode data code " << err << "\n";
        }

        data_container.root_raw_data = good_data.returnRawData();

        std::cout << "RAW data size is " << data_container.root_raw_data.size()
                  << "\n";

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion();
        if (err != 0)
            std::cout << "Make conversion error code " << err << "\n";

        /* err = loc_conv_uvw.substractBl();

        if (err != 0)
            std::cout << "Substractbl error code " << err << "\n"; */

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        std::cout << "UVW data size is " << data_container.uvw_data.size()
                  << "\n";

        /* reorderCh::reorderChfromFile<reorderCh::planeInfo_U>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_V>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_W>(
            data_container.uvw_data);
 */
        loc_clean_uvw.setUVWData(data_container.uvw_data);

        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoU>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoV>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoW>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;

        data_container.uvw_data = loc_clean_uvw.returnDataUVW();

        err = loc_convert_hit.setUVWData(data_container.uvw_data);
        if (err != 0)
            std::cout << "Error set UVW data code " << err << "\n";

        err = loc_convert_hit.getHitInfo();
        if (err != 0)
            std::cout << "Error get hit info code " << err << "\n";

        data_container.hit_data = loc_convert_hit.returnHitData();
        data_container.raw_hist_container = loc_convert_hit.returnHistData();

        std::cout << "Hit data size " << data_container.hit_data.size() << "\n";
        std::cout << "Hist data size "
                  << data_container.raw_hist_container.size() << "\n";

        err = loc_conv_xyz.getNewVector(data_container.hit_data);
        if (err != 0)
            std::cout << "Error get new vector code " << err << "\n";

        err = loc_conv_xyz.makeConversionXYZ();
        if (err != 0)
            std::cout << "Error make conversion XYZ code " << err << "\n";

        data_container.xyz_data = loc_conv_xyz.returnXYZ();

        std::cout << "XYZ vector size " << data_container.xyz_data.size()
                  << "\n";

        if (u_hists != nullptr)
            u_hists->Reset();

        if (v_hists != nullptr)
            v_hists->Reset();

        if (w_hists != nullptr)
            w_hists->Reset();

        if (u_graph != nullptr)
            u_graph->Delete();

        if (v_graph != nullptr)
            v_graph->Delete();

        if (w_graph != nullptr)
            w_graph->Delete();

        if (p_graph != nullptr)
            p_graph->Delete();

        if (p_graph3d != nullptr)
            p_graph3d->Delete();

        u_hists = new TH2D(Form("u_hists_%d", entry_nr),
                           Form("Histogram for U entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        v_hists = new TH2D(Form("v_hists_%d", entry_nr),
                           Form("Histogram for V entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        w_hists = new TH2D(Form("w_hists_%d", entry_nr),
                           Form("Histogram for W entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);

        int bin = 0;

        for (const auto &iter : data_container.uvw_data) {

            bin = 0;

            if (iter.plane_val == 0) {

                for (const auto &sig_iter : iter.signal_val) {

                    u_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 1) {

                for (const auto &sig_iter : iter.signal_val) {

                    v_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 2) {

                for (const auto &sig_iter : iter.signal_val) {

                    w_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }
        }

        loc_pad->cd(1);
        u_hists->Draw("COLZ");
        loc_pad->cd(2);
        v_hists->Draw("COLZ");
        loc_pad->cd(3);
        w_hists->Draw("COLZ");

        std::vector<double> x_u;
        std::vector<double> y_u;
        std::vector<double> c_u;

        std::vector<double> x_v;
        std::vector<double> y_v;
        std::vector<double> c_v;

        std::vector<double> x_w;
        std::vector<double> y_w;
        std::vector<double> c_w;

        for (const auto &hit_iter : data_container.hit_data) {

            if (hit_iter.plane == 0) {

                x_u.push_back(hit_iter.peak_x);
                y_u.push_back(hit_iter.strip);
                c_u.push_back(hit_iter.peak_y + hit_iter.base_line);

            } else if (hit_iter.plane == 1) {

                x_v.push_back(hit_iter.peak_x);
                y_v.push_back(hit_iter.strip);
                c_v.push_back(hit_iter.peak_y + hit_iter.base_line);

            } else if (hit_iter.plane == 2) {

                x_w.push_back(hit_iter.peak_x);
                y_w.push_back(hit_iter.strip);
                c_w.push_back(hit_iter.peak_y + hit_iter.base_line);
            }
        }

        if (data_container.hit_data.size() != 0) {

            u_graph = new TGraph(x_u.size(), x_u.data(), y_u.data());
            u_graph->GetXaxis()->SetLimits(-1, 512);
            u_graph->GetHistogram()->SetMaximum(73);
            u_graph->GetHistogram()->SetMinimum(0);
            // u_graph->SetMarkerColor(kBlack);
            u_graph->SetMarkerStyle(kFullCircle);
            u_graph->SetTitle("Hits detected on U plane; Time; Strip");
            // Set marker colors Pink > Red > Green > Blue
            if (c_u.size() != 0) {

                auto min_val_c_u = 0;
                auto max_val_c_u = 2000;

                std::transform(c_u.begin(), c_u.end(), c_u.begin(),
                               [min_val_c_u, max_val_c_u](const double x) {
                                   return (x - min_val_c_u) /
                                          (max_val_c_u - min_val_c_u);
                               });

                for (auto vec_i = 0; vec_i < c_u.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    u_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();

                    if (c_u.at(vec_i) < 0.33) {
                        TColor *loc_color = new TColor(0, 0, 3 * c_u.at(vec_i));
                    } else if (c_u.at(vec_i) >= 0.33 && c_u.at(vec_i) < 0.66) {
                        TColor *loc_color =
                            new TColor(0, 3 * c_u.at(vec_i) / 2, 0);
                    } else if (c_u.at(vec_i) > 1) {
                        TColor *loc_color = new TColor(1, 0, 1);
                    } else {
                        TColor *loc_color = new TColor(c_u.at(vec_i), 0, 0);
                    }

                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    u_graph->GetListOfFunctions()->Add(loc_marker);
                }
            }
            loc_pad->cd(4);
            u_graph->Draw("AP");

            v_graph = new TGraph(x_v.size(), x_v.data(), y_v.data());
            v_graph->GetXaxis()->SetLimits(-1, 512);
            v_graph->GetHistogram()->SetMaximum(93);
            v_graph->GetHistogram()->SetMinimum(0);
            // v_graph->SetMarkerColor(kBlack);
            v_graph->SetMarkerStyle(kFullCircle);
            v_graph->SetTitle("Hits detected on V plane; Time; Strip");
            if (c_v.size() != 0) {

                auto min_val_c_v = 0;
                auto max_val_c_v = 2000;

                std::transform(c_v.begin(), c_v.end(), c_v.begin(),
                               [min_val_c_v, max_val_c_v](const double x) {
                                   return (x - min_val_c_v) /
                                          (max_val_c_v - min_val_c_v);
                               });

                for (auto vec_i = 0; vec_i < c_v.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    v_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();

                    if (c_v.at(vec_i) < 0.33) {
                        TColor *loc_color = new TColor(0, 0, 3 * c_v.at(vec_i));
                    } else if (c_v.at(vec_i) >= 0.33 && c_v.at(vec_i) < 0.66) {
                        TColor *loc_color =
                            new TColor(0, 3 * c_v.at(vec_i) / 2, 0);
                    } else if (c_v.at(vec_i) > 1) {
                        TColor *loc_color = new TColor(1, 0, 1);
                    } else {
                        TColor *loc_color = new TColor(c_v.at(vec_i), 0, 0);
                    }

                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    v_graph->GetListOfFunctions()->Add(loc_marker);
                }
            }
            loc_pad->cd(5);
            v_graph->Draw("AP");

            w_graph = new TGraph(x_w.size(), x_w.data(), y_w.data());
            w_graph->GetXaxis()->SetLimits(-1, 512);
            w_graph->GetHistogram()->SetMaximum(93);
            w_graph->GetHistogram()->SetMinimum(0);
            // w_graph->SetMarkerColor(kBlack);
            w_graph->SetMarkerStyle(kFullCircle);
            w_graph->SetTitle("Hits detected on W plane; Time; Strip");
            if (c_w.size() != 0) {

                auto min_val_c_w = 0;
                auto max_val_c_w = 2000;

                std::transform(c_w.begin(), c_w.end(), c_w.begin(),
                               [min_val_c_w, max_val_c_w](const double x) {
                                   return (x - min_val_c_w) /
                                          (max_val_c_w - min_val_c_w);
                               });

                for (auto vec_i = 0; vec_i < c_w.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    w_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();

                    if (c_w.at(vec_i) < 0.33) {
                        TColor *loc_color = new TColor(0, 0, 3 * c_w.at(vec_i));
                    } else if (c_w.at(vec_i) >= 0.33 && c_w.at(vec_i) < 0.66) {
                        TColor *loc_color =
                            new TColor(0, 3 * c_w.at(vec_i) / 2, 0);
                    } else if (c_w.at(vec_i) > 1) {
                        TColor *loc_color = new TColor(1, 0, 1);
                    } else {
                        TColor *loc_color = new TColor(c_w.at(vec_i), 0, 0);
                    }

                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    w_graph->GetListOfFunctions()->Add(loc_marker);
                }
            }
            loc_pad->cd(6);
            w_graph->Draw("AP");

            loc_pad->Modified();
            loc_pad->Update();

        } else {
            u_graph = nullptr;
            v_graph = nullptr;
            w_graph = nullptr;
        }

        if (data_container.xyz_data.size() != 0) {

            std::vector<double> x, y, z, chg;

            x.push_back(-10.0);
            y.push_back(-10.0);
            z.push_back(-10.0);
            chg.push_back(0.0);

            for (const auto &point_iter : data_container.xyz_data) {

                x.push_back(point_iter.data_x);
                y.push_back(point_iter.data_y);
                z.push_back(point_iter.data_z);
                chg.push_back(point_iter.data_charge);
            }

            x.push_back(150.0);
            y.push_back(150.0);
            z.push_back(150.0);
            chg.push_back(0.0);

            p_graph = new TGraph(x.size(), x.data(), y.data());
            p_graph->GetXaxis()->SetLimits(-10, 150);
            p_graph->GetHistogram()->SetMaximum(150);
            p_graph->GetHistogram()->SetMinimum(-10);
            p_graph->SetMarkerStyle(kFullCircle);
            p_graph->SetTitle("XY coordinates projection; X axis; Y axis");
            if (chg.size() != 0) {

                auto min_val_chg = 0;
                auto max_val_chg = 6000;

                std::transform(chg.begin(), chg.end(), chg.begin(),
                               [min_val_chg, max_val_chg](const double x) {
                                   return (x - min_val_chg) /
                                          (max_val_chg - min_val_chg);
                               });

                for (auto vec_i = 0; vec_i < chg.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    p_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();

                    if (chg.at(vec_i) < 0.33) {
                        TColor *loc_color = new TColor(0, 0, 3 * chg.at(vec_i));
                    } else if (chg.at(vec_i) >= 0.33 && chg.at(vec_i) < 0.66) {
                        TColor *loc_color =
                            new TColor(0, 3 * chg.at(vec_i) / 2, 0);
                    } else if (chg.at(vec_i) > 1) {
                        TColor *loc_color = new TColor(1, 0, 1);
                    } else {
                        TColor *loc_color = new TColor(chg.at(vec_i), 0, 0);
                    }

                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    p_graph->GetListOfFunctions()->Add(loc_marker);
                }
            }
            loc_pad->cd(7);
            p_graph->Draw("AP");
            loc_pad->Modified();
            loc_pad->Update();

            p_graph3d = new TGraph2D(x.size(), x.data(), y.data(), z.data());
            p_graph3d->SetMarkerColor(kBlue);
            p_graph3d->SetMarkerStyle(kFullCircle);
            p_graph3d->SetTitle("Reconstructed data in XYZ coordinates; X "
                                "axis; Y axis; Z axis");
            loc_pad->cd(8);
            p_graph3d->Draw("P0");
            loc_pad->Modified();
            loc_pad->Update();

        } else {
            p_graph = nullptr;
            p_graph3d = nullptr;
        }

        entry_nr++;

        loc_pad->Update();
        loc_canv->Update();
        loc_canv->Print(destination_file);
    }

    gErrorIgnoreLevel = kPrint;
    loc_canv->Print(destination_file + "]");

    loc_canv->Close();

    if (loc_canv) {
        delete (loc_canv);
        loc_canv = nullptr;
    }

    gROOT->SetBatch(kFALSE);

    std::cout << "\n\n\nDONE!!!!!\n\n\n" << std::endl;
}

/**
 * @brief Create a pdf file with the raw data entries/
 *
 * @param source_file The source .root file.
 * @param destination_file The name and loation of the pdf file to be created.
 * @param read_entries The number of entries read.
 */
void create_raw_pdf(TString source_file, TString destination_file,
                    int read_entries) {

    int entry_nr = 0;
    int max_entries;

    auto goodFile = source_file;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    max_entries = good_data.returnNEntries();

    convertUVW loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    convertHitData loc_convert_hit;

    convertXYZ loc_conv_xyz;

    gROOT->SetBatch(kTRUE);

    auto loc_canv = new TCanvas("entries.pdf", "PDF", 1500, 1000);
    auto loc_pad = new TPad("pad name", "pad title", 0, 0, 1, 1);
    loc_pad->Divide(3, 1);
    loc_pad->Draw();

    TH2D *u_hists = nullptr;
    TH2D *v_hists = nullptr;
    TH2D *w_hists = nullptr;

    loc_canv->Print(destination_file + "[");
    gErrorIgnoreLevel = kWarning;

    // only read the number of entries wanted
    max_entries = std::min(max_entries, read_entries);

    while (entry_nr < max_entries) {

        generalDataStorage data_container;

        std::cout << "\n\n\nNow at entry: " << entry_nr << " of " << max_entries
                  << " for " << destination_file << "\n";

        err = good_data.decodeData(entry_nr);
        if (err != 0) {
            std::cout << "Error decode data code " << err << "\n";
        }

        data_container.root_raw_data = good_data.returnRawData();

        std::cout << "RAW data size is " << data_container.root_raw_data.size()
                  << "\n";

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion();
        if (err != 0)
            std::cout << "Make conversion error code " << err << "\n";

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        std::cout << "UVW data size is " << data_container.uvw_data.size()
                  << "\n";

        if (u_hists != nullptr)
            u_hists->Reset();

        if (v_hists != nullptr)
            v_hists->Reset();

        if (w_hists != nullptr)
            w_hists->Reset();

        u_hists = new TH2D(Form("u_hists_%d", entry_nr),
                           Form("Histogram for U entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        v_hists = new TH2D(Form("v_hists_%d", entry_nr),
                           Form("Histogram for V entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        w_hists = new TH2D(Form("w_hists_%d", entry_nr),
                           Form("Histogram for W entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);

        int bin = 0;
        // int strip = 1;

        for (const auto &iter : data_container.uvw_data) {

            bin = 0;

            if (iter.plane_val == 0) {

                for (const auto &sig_iter : iter.signal_val) {

                    u_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 1) {

                for (const auto &sig_iter : iter.signal_val) {

                    v_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 2) {

                for (const auto &sig_iter : iter.signal_val) {

                    w_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }
        }

        loc_pad->cd(1);
        u_hists->Draw("COLZ");
        loc_pad->cd(2);
        v_hists->Draw("COLZ");
        loc_pad->cd(3);
        w_hists->Draw("COLZ");

        entry_nr++;

        loc_pad->Update();
        loc_canv->Update();
        loc_canv->Print(destination_file);
    }

    gErrorIgnoreLevel = kPrint;
    loc_canv->Print(destination_file + "]");

    loc_canv->Close();

    if (loc_canv) {
        delete (loc_canv);
        loc_canv = nullptr;
    }

    gROOT->SetBatch(kFALSE);

    std::cout << "\n\n\nDONE!!!!!\n\n\n" << std::endl;
}

/**
 * @brief Writes the calculated XYZ values from the given entry from the given
 * file to a .csv file.
 *
 * @param filename The name of the .root file.
 * @param entry_nr The entry number to be converted.
 */
void writeXYZcvs(TString filename = "./rootdata/data2.root",
                 int entry_nr = 429) {

    generalDataStorage data_container;

    auto goodFile = filename;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    err = good_data.decodeData(entry_nr);
    data_container.root_raw_data = good_data.returnRawData();

    convertUVW loc_conv_uvw(data_container.root_raw_data);

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    err = loc_conv_uvw.makeConversion();
    if (err != 0)
        std::cout << "Make conversion error code " << err << std::endl;

    /* err = loc_conv_uvw.substractBl();

    if (err != 0)
        std::cout << "Substractbl error code " << err << std::endl; */

    /* err = loc_conv_uvw.drawChargeHist();

    if (err != 0)
        std::cout << "Draw Charge Hist error code " << err << std::endl; */

    data_container.uvw_data = loc_conv_uvw.returnDataUVW();

    convertHitData loc_convert_hit(data_container.uvw_data);

    err = loc_convert_hit.getHitInfo();
    if (err != 0)
        std::cout << "Error get hit info code " << err << std::endl;

    data_container.hit_data = loc_convert_hit.returnHitData();
    data_container.raw_hist_container = loc_convert_hit.returnHistData();

    std::cout << "Hit data size " << data_container.hit_data.size()
              << std::endl;
    std::cout << "Hist data size " << data_container.raw_hist_container.size()
              << std::endl;

    convertXYZ loc_conv_xyz(data_container.hit_data);

    err = loc_conv_xyz.makeConversionXYZ();

    data_container.xyz_data = loc_conv_xyz.returnXYZ();

    std::ofstream out_file("./converteddata/test_xyz.csv");

    if (!out_file.is_open()) {
        std::cerr << "Error: Could not open file for output" << std::endl;
    }

    // header
    out_file << "x,y,z\n";

    for (const auto &data_entry : data_container.xyz_data) {

        out_file << data_entry.data_x << "," << data_entry.data_y << ","
                 << data_entry.data_z;

        out_file << "\n";
    }

    out_file.close();
}

/**
 * @brief Converts all of the entries to XYZ and writes them in a .csv file.
 *
 * @param filename The name of the .root file.
 */
void writeFullXYZCSV(TString filename = "./rootdata/data2.root") {

    auto goodFile = filename;

    auto goodTree = "tree";

    std::ofstream out_file("./converteddata/data2allentries.csv");

    if (!out_file.is_open()) {
        std::cerr << "Error: Could not open file for output" << std::endl;
        return;
    }

    loadData good_data(goodFile, goodTree);

    convertUVW loc_conv_uvw;

    cleanUVW loc_clean_uvw;

    convertHitData loc_convert_hit;

    convertXYZ loc_conv_xyz;

    auto err = good_data.openFile();
    err = good_data.readData();

    err = loc_conv_uvw.openSpecFile();

    auto max_entries = good_data.returnNEntries();

    // header
    out_file << "x,y,z,entry_nr\n";

    int entry_nr = 0;

    while (entry_nr < max_entries) {

        std::cout << "\n\n\nNow at entry: " << entry_nr << " of " << max_entries
                  << "\n";

        generalDataStorage data_container;

        err = good_data.decodeData(entry_nr);
        if (err != 0) {
            std::cout << "Error decode data code " << err << "\n";
        }

        data_container.root_raw_data = good_data.returnRawData();

        std::cout << "RAW data size is " << data_container.root_raw_data.size()
                  << "\n";

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion();
        if (err != 0)
            std::cout << "Make conversion error code " << err << "\n";

        /* err = loc_conv_uvw.substractBl();

        if (err != 0)
            std::cout << "Substractbl error code " << err << "\n"; */

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        std::cout << "UVW data size is " << data_container.uvw_data.size()
                  << "\n";

        /* reorderCh::reorderChfromFile<reorderCh::planeInfo_U>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_V>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_W>(
            data_container.uvw_data); */

        loc_clean_uvw.setUVWData(data_container.uvw_data);

        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoU>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoV>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoW>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;

        data_container.uvw_data = loc_clean_uvw.returnDataUVW();

        err = loc_convert_hit.setUVWData(data_container.uvw_data);
        if (err != 0)
            std::cout << "Error set UVW data code " << err << "\n";

        err = loc_convert_hit.getHitInfo();
        if (err != 0)
            std::cout << "Error get hit info code " << err << "\n";

        data_container.hit_data = loc_convert_hit.returnHitData();
        data_container.raw_hist_container = loc_convert_hit.returnHistData();

        std::cout << "Hit data size " << data_container.hit_data.size() << "\n";
        std::cout << "Hist data size "
                  << data_container.raw_hist_container.size() << "\n";

        err = loc_conv_xyz.getNewVector(data_container.hit_data);
        if (err != 0)
            std::cout << "Error get new vector code " << err << "\n";

        err = loc_conv_xyz.makeConversionXYZ();
        if (err != 0)
            std::cout << "Error make conversion XYZ code " << err << "\n";

        data_container.xyz_data = loc_conv_xyz.returnXYZ();

        std::cout << "XYZ vector size " << data_container.xyz_data.size()
                  << "\n";

        for (const auto &data_entry : data_container.xyz_data) {

            out_file << data_entry.data_x << "," << data_entry.data_y << ","
                     << data_entry.data_z << "," << entry_nr;

            out_file << "\n";
        }

        entry_nr++;
    }

    out_file.close();
}

/**
 * @brief Draws an XY image of the entry from the .root file specified.
 *
 * @param filename The name of the .root file.
 * @param entry_nr The entry number to be converted.
 */
void drawXYimage(TString filename = "./rootdata/data2.root",
                 int entry_nr = 429) {

    generalDataStorage data_container;

    auto goodFile = filename;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    err = good_data.decodeData(entry_nr);
    data_container.root_raw_data = good_data.returnRawData();

    convertUVW loc_conv_uvw(data_container.root_raw_data);

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    err = loc_conv_uvw.makeConversion();
    if (err != 0)
        std::cout << "Make conversion error code " << err << std::endl;

    /* err = loc_conv_uvw.substractBl();

    if (err != 0)
        std::cout << "Substractbl error code " << err << std::endl;

    err = loc_conv_uvw.drawChargeHist();

    if (err != 0)
        std::cout << "Draw Charge Hist error code " << err << std::endl; */

    data_container.uvw_data = loc_conv_uvw.returnDataUVW();

    convertHitData loc_convert_hit(data_container.uvw_data);

    err = loc_convert_hit.getHitInfo();
    if (err != 0)
        std::cout << "Error get hit info code " << err << std::endl;

    data_container.hit_data = loc_convert_hit.returnHitData();
    data_container.raw_hist_container = loc_convert_hit.returnHistData();

    std::cout << "Hit data size " << data_container.hit_data.size()
              << std::endl;
    std::cout << "Hist data size " << data_container.raw_hist_container.size()
              << std::endl;

    convertXYZ loc_conv_xyz(data_container.hit_data);

    err = loc_conv_xyz.makeConversionXYZ();

    data_container.xyz_data = loc_conv_xyz.returnXYZ();

    std::vector<double> x, y, z;

    for (const auto &point_iter : data_container.xyz_data) {

        x.push_back(point_iter.data_x);
        y.push_back(point_iter.data_y);
        z.push_back(point_iter.data_z);
    }

    auto loc_canv = new TCanvas("xy format", "", 800, 600);
    loc_canv->SetFrameLineColor(0);
    loc_canv->SetFrameLineWidth(0);
    loc_canv->SetBottomMargin(0);
    loc_canv->SetTopMargin(0);
    loc_canv->SetLeftMargin(0);
    loc_canv->SetRightMargin(0);

    gStyle->SetOptStat(0);

    auto p_graph = new TGraph(x.size(), x.data(), y.data());

    p_graph->GetXaxis()->SetTickLength(0);
    p_graph->GetXaxis()->SetLabelSize(0);
    p_graph->GetYaxis()->SetTickLength(0);
    p_graph->GetYaxis()->SetLabelSize(0);

    p_graph->GetXaxis()->SetLimits(-10, 150);
    p_graph->GetHistogram()->SetMaximum(150);
    p_graph->GetHistogram()->SetMinimum(-10);
    p_graph->SetMarkerColor(kBlue);
    p_graph->SetMarkerStyle(kFullCircle);
    p_graph->SetTitle("");
    p_graph->Draw("AP");

    loc_canv->Update();
    loc_canv->Print("./converteddata/test_xyz.png");

    loc_canv->Close();
}

/**
 * @brief Allows the viewing of the data from the .root file in 3 formats:
 * - UVW with the baseline extracted.
 * - Hits detected for each plane.
 * - The reconstructed XYZ points.
 *
 * @param fileName The name of the .root file.
 * @param norm_opt The normalization option. 1 if you want the channels to be
 * normalized, else 0.
 */
void view_data_entries(TString fileName, const bool charge_opt = false) {

    int entry_nr = -1;
    int max_entries;

    auto goodFile = fileName;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    max_entries = good_data.returnNEntries();

    convertUVW loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    cleanUVW loc_clean_uvw;

    convertHitData loc_convert_hit;

    convertXYZ loc_conv_xyz;

    auto loc_canv =
        new TCanvas("View entries canvas", "View entries", 1500, 1000);
    auto loc_pad = new TPad("pad name", "pad title", 0, 0, 1, 1);
    loc_pad->Divide(3, 3);
    loc_pad->Draw();

    TH2D *u_hists = nullptr;
    TH2D *v_hists = nullptr;
    TH2D *w_hists = nullptr;

    TGraph *u_graph = nullptr;
    TGraph *v_graph = nullptr;
    TGraph *w_graph = nullptr;

    TGraph *p_graph = nullptr;
    TGraph2D *p_graph3d = nullptr;

    uint32_t loop_nr = 0;

    while (1) {

        generalDataStorage data_container;

        std::cout
            << "Press 'e' to exit, 'd' for next and 'a' for previous and i "
               "to input the entry number. Loop "
            << loop_nr << std::endl;

        loop_nr++;

        char key_val = getchar();

        // Clear the input buffer
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (key_val == 'e') {

            break;

        } else if (key_val == 'd') {

            if (entry_nr < max_entries) {
                entry_nr++;
            }

            std::cout << "Entry number is " << entry_nr << "\n\n" << std::endl;

        } else if (key_val == 'a') {

            if (entry_nr > 0) {
                entry_nr--;
            }

            std::cout << "Entry number is " << entry_nr << "\n\n" << std::endl;

        } else if (key_val == 'i') {

            std::cout << "Please enter the entry number.\n";

            std::cin >> entry_nr;

            if (entry_nr < 0 || entry_nr > (max_entries - 1))
                entry_nr = 0;

            std::cout << "Entry number is " << entry_nr << "\n\n" << std::endl;
        }

        if (entry_nr < 0) {
            entry_nr = 0;
        }

        err = good_data.decodeData(entry_nr);
        if (err != 0) {
            std::cout << "Error decode data code " << err << std::endl;
        }

        data_container.root_raw_data = good_data.returnRawData();

        std::cout << "RAW data size is " << data_container.root_raw_data.size()
                  << std::endl;

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion();
        if (err != 0)
            std::cout << "Make conversion error code " << err << std::endl;

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        std::cout << "UVW data size is " << data_container.uvw_data.size()
                  << std::endl;

        /* reorderCh::reorderChfromFile<reorderCh::planeInfo_U>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_V>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_W>(
            data_container.uvw_data);
 */

        loc_clean_uvw.setUVWData(data_container.uvw_data);

        if (charge_opt) {

            auto *charge_canv = new TCanvas("Charge_canvas", "Charge_canvas");
            auto charge_pad = new TPad("charge_pad", "Charge pad", 0, 0, 1, 1);
            charge_pad->Divide(3, 1);
            charge_pad->Draw();

            TH1D *charge_u;
            TH1D *charge_v;
            TH1D *charge_w;

            err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoU>();
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;
            err =
                loc_clean_uvw.getChargeHist<cleanUVW::miniPlaneInfoU>(charge_u);
            if (err != 0)
                std::cerr << "Error getChargeHist code " << err << std::endl;
            err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoV>();
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;
            err =
                loc_clean_uvw.getChargeHist<cleanUVW::miniPlaneInfoV>(charge_v);
            if (err != 0)
                std::cerr << "Error getChargeHist code " << err << std::endl;
            err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoW>();
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;
            err =
                loc_clean_uvw.getChargeHist<cleanUVW::miniPlaneInfoW>(charge_w);
            if (err != 0)
                std::cerr << "Error getChargeHist code " << err << std::endl;

            charge_pad->cd(1);
            charge_u->Draw();
            charge_pad->cd(2);
            charge_v->Draw();
            charge_pad->cd(3);
            charge_w->Draw();

            charge_canv->Update();

        } else {

            err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoU>();
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;

            err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoV>();
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;

            err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoW>();
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;
        }

        data_container.uvw_data = loc_clean_uvw.returnDataUVW();

        err = loc_convert_hit.setUVWData(data_container.uvw_data);
        if (err != 0)
            std::cout << "Error set UVW data code " << err << std::endl;

        err = loc_convert_hit.getHitInfo(2.5, 0.3);
        if (err != 0)
            std::cout << "Error get hit info code " << err << std::endl;

        try {
            if (loc_convert_hit.containsVerticalLine() == true) {

                std::cout << "\n\nContains vertical line!\n\n";
            }
        } catch (std::exception &e) {

            std::cerr << "Exception detected at containsVerticalLine: \n\t"
                      << e.what() << std::endl;
        }

        data_container.hit_data = loc_convert_hit.returnHitData();
        // data_container.raw_hist_container = loc_convert_hit.returnHistData();

        std::cout << "Hit data size " << data_container.hit_data.size()
                  << std::endl;
        /* std::cout << "Hist data size "
                  << data_container.raw_hist_container.size() << std::endl; */

        err = loc_conv_xyz.getNewVector(data_container.hit_data);
        if (err != 0)
            std::cout << "Error get new vector code " << err << std::endl;

        err = loc_conv_xyz.makeConversionXYZ();
        if (err != 0)
            std::cout << "Error make conversion XYZ code " << err << std::endl;

        data_container.xyz_data = loc_conv_xyz.returnXYZ();

        std::cout << "XYZ vector size " << data_container.xyz_data.size()
                  << std::endl;

        if (u_hists != nullptr)
            u_hists->Reset();

        if (v_hists != nullptr)
            v_hists->Reset();

        if (w_hists != nullptr)
            w_hists->Reset();

        if (u_graph != nullptr)
            u_graph->Delete();

        if (v_graph != nullptr)
            v_graph->Delete();

        if (w_graph != nullptr)
            w_graph->Delete();

        if (p_graph != nullptr)
            p_graph->Delete();

        if (p_graph3d != nullptr)
            p_graph3d->Delete();

        u_hists = new TH2D(Form("u_hists_%d", entry_nr),
                           Form("Histogram for U entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        v_hists = new TH2D(Form("v_hists_%d", entry_nr),
                           Form("Histogram for V entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        w_hists = new TH2D(Form("w_hists_%d", entry_nr),
                           Form("Histogram for W entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);

        int bin = 0;

        for (const auto &iter : data_container.uvw_data) {

            bin = 0;

            if (iter.plane_val == 0) {

                for (const auto &sig_iter : iter.signal_val) {

                    u_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 1) {

                for (const auto &sig_iter : iter.signal_val) {

                    v_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 2) {

                for (const auto &sig_iter : iter.signal_val) {

                    w_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }
        }

        loc_pad->cd(1);
        u_hists->Draw("COLZ");
        loc_pad->cd(2);
        v_hists->Draw("COLZ");
        loc_pad->cd(3);
        w_hists->Draw("COLZ");

        std::vector<double> x_u;
        std::vector<double> y_u;
        std::vector<double> c_u;

        std::vector<double> x_v;
        std::vector<double> y_v;
        std::vector<double> c_v;

        std::vector<double> x_w;
        std::vector<double> y_w;
        std::vector<double> c_w;

        for (const auto &hit_iter : data_container.hit_data) {

            if (hit_iter.plane == 0) {

                x_u.push_back(hit_iter.peak_x);
                y_u.push_back(hit_iter.strip);
                c_u.push_back(hit_iter.peak_y + hit_iter.base_line);

            } else if (hit_iter.plane == 1) {

                x_v.push_back(hit_iter.peak_x);
                y_v.push_back(hit_iter.strip);
                c_v.push_back(hit_iter.peak_y + hit_iter.base_line);

            } else if (hit_iter.plane == 2) {

                x_w.push_back(hit_iter.peak_x);
                y_w.push_back(hit_iter.strip);
                c_w.push_back(hit_iter.peak_y + hit_iter.base_line);
            }
        }

        if (data_container.hit_data.size() != 0) {

            u_graph = new TGraph(x_u.size(), x_u.data(), y_u.data());
            u_graph->GetXaxis()->SetLimits(-1, 512);
            u_graph->GetHistogram()->SetMaximum(73);
            u_graph->GetHistogram()->SetMinimum(0);
            // u_graph->SetMarkerColor(kBlack);
            u_graph->SetMarkerStyle(kFullCircle);
            u_graph->SetTitle("Hits detected on U plane; Time; Strip");
            // Set marker colors Pink > Red > Green > Blue
            if (c_u.size() != 0) {

                auto min_val_c_u = 0;
                auto max_val_c_u = 2000;

                std::transform(c_u.begin(), c_u.end(), c_u.begin(),
                               [min_val_c_u, max_val_c_u](const double x) {
                                   return (x - min_val_c_u) /
                                          (max_val_c_u - min_val_c_u);
                               });

                for (auto vec_i = 0; vec_i < c_u.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    u_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();

                    if (c_u.at(vec_i) < 0.33) {
                        TColor *loc_color = new TColor(0, 0, 3 * c_u.at(vec_i));
                    } else if (c_u.at(vec_i) >= 0.33 && c_u.at(vec_i) < 0.66) {
                        TColor *loc_color =
                            new TColor(0, 3 * c_u.at(vec_i) / 2, 0);
                    } else if (c_u.at(vec_i) > 1) {
                        TColor *loc_color = new TColor(1, 0, 1);
                    } else {
                        TColor *loc_color = new TColor(c_u.at(vec_i), 0, 0);
                    }

                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    u_graph->GetListOfFunctions()->Add(loc_marker);
                }
            }
            loc_pad->cd(4);
            u_graph->Draw("AP");

            v_graph = new TGraph(x_v.size(), x_v.data(), y_v.data());
            v_graph->GetXaxis()->SetLimits(-1, 512);
            v_graph->GetHistogram()->SetMaximum(93);
            v_graph->GetHistogram()->SetMinimum(0);
            // v_graph->SetMarkerColor(kBlack);
            v_graph->SetMarkerStyle(kFullCircle);
            v_graph->SetTitle("Hits detected on V plane; Time; Strip");
            if (c_v.size() != 0) {

                auto min_val_c_v = 0;
                auto max_val_c_v = 2000;

                std::transform(c_v.begin(), c_v.end(), c_v.begin(),
                               [min_val_c_v, max_val_c_v](const double x) {
                                   return (x - min_val_c_v) /
                                          (max_val_c_v - min_val_c_v);
                               });

                for (auto vec_i = 0; vec_i < c_v.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    v_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();

                    if (c_v.at(vec_i) < 0.33) {
                        TColor *loc_color = new TColor(0, 0, 3 * c_v.at(vec_i));
                    } else if (c_v.at(vec_i) >= 0.33 && c_v.at(vec_i) < 0.66) {
                        TColor *loc_color =
                            new TColor(0, 3 * c_v.at(vec_i) / 2, 0);
                    } else if (c_v.at(vec_i) > 1) {
                        TColor *loc_color = new TColor(1, 0, 1);
                    } else {
                        TColor *loc_color = new TColor(c_v.at(vec_i), 0, 0);
                    }

                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    v_graph->GetListOfFunctions()->Add(loc_marker);
                }
            }
            loc_pad->cd(5);
            v_graph->Draw("AP");

            w_graph = new TGraph(x_w.size(), x_w.data(), y_w.data());
            w_graph->GetXaxis()->SetLimits(-1, 512);
            w_graph->GetHistogram()->SetMaximum(93);
            w_graph->GetHistogram()->SetMinimum(0);
            // w_graph->SetMarkerColor(kBlack);
            w_graph->SetMarkerStyle(kFullCircle);
            w_graph->SetTitle("Hits detected on W plane; Time; Strip");
            if (c_w.size() != 0) {

                auto min_val_c_w = 0;
                auto max_val_c_w = 2000;

                std::transform(c_w.begin(), c_w.end(), c_w.begin(),
                               [min_val_c_w, max_val_c_w](const double x) {
                                   return (x - min_val_c_w) /
                                          (max_val_c_w - min_val_c_w);
                               });

                for (auto vec_i = 0; vec_i < c_w.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    w_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();

                    if (c_w.at(vec_i) < 0.33) {
                        TColor *loc_color = new TColor(0, 0, 3 * c_w.at(vec_i));
                    } else if (c_w.at(vec_i) >= 0.33 && c_w.at(vec_i) < 0.66) {
                        TColor *loc_color =
                            new TColor(0, 3 * c_w.at(vec_i) / 2, 0);
                    } else if (c_w.at(vec_i) > 1) {
                        TColor *loc_color = new TColor(1, 0, 1);
                    } else {
                        TColor *loc_color = new TColor(c_w.at(vec_i), 0, 0);
                    }

                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    w_graph->GetListOfFunctions()->Add(loc_marker);
                }
            }
            loc_pad->cd(6);
            w_graph->Draw("AP");

            loc_pad->Modified();
            loc_pad->Update();

        } else {
            u_graph = nullptr;
            v_graph = nullptr;
            w_graph = nullptr;
        }

        if (data_container.xyz_data.size() != 0) {

            std::vector<double> x, y, z, chg;

            x.push_back(-10.0);
            y.push_back(-10.0);
            z.push_back(-10.0);
            chg.push_back(0.0);

            for (const auto &point_iter : data_container.xyz_data) {

                x.push_back(point_iter.data_x);
                y.push_back(point_iter.data_y);
                z.push_back(point_iter.data_z);
                chg.push_back(point_iter.data_charge);
            }

            x.push_back(150.0);
            y.push_back(150.0);
            z.push_back(150.0);
            chg.push_back(0.0);

            p_graph = new TGraph(x.size(), x.data(), y.data());
            p_graph->GetXaxis()->SetLimits(-10, 150);
            p_graph->GetHistogram()->SetMaximum(150);
            p_graph->GetHistogram()->SetMinimum(-10);
            // p_graph->SetMarkerColor(kBlack);
            p_graph->SetMarkerStyle(kFullCircle);
            p_graph->SetTitle("XY coordinates projection; X axis; Y axis");
            if (chg.size() != 0) {

                auto min_val_chg = 0;
                auto max_val_chg = 6000;

                std::transform(chg.begin(), chg.end(), chg.begin(),
                               [min_val_chg, max_val_chg](const double x) {
                                   return (x - min_val_chg) /
                                          (max_val_chg - min_val_chg);
                               });

                for (auto vec_i = 0; vec_i < chg.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    p_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();

                    if (chg.at(vec_i) < 0.33) {
                        TColor *loc_color = new TColor(0, 0, 3 * chg.at(vec_i));
                    } else if (chg.at(vec_i) >= 0.33 && chg.at(vec_i) < 0.66) {
                        TColor *loc_color =
                            new TColor(0, 3 * chg.at(vec_i) / 2, 0);
                    } else if (chg.at(vec_i) > 1) {
                        TColor *loc_color = new TColor(1, 0, 1);
                    } else {
                        TColor *loc_color = new TColor(chg.at(vec_i), 0, 0);
                    }

                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    p_graph->GetListOfFunctions()->Add(loc_marker);
                }
            }
            loc_pad->cd(7);
            p_graph->Draw("AP");
            loc_pad->Modified();
            loc_pad->Update();

            p_graph3d = new TGraph2D(x.size(), x.data(), y.data(), z.data());
            p_graph3d->SetMarkerColor(kBlue);
            p_graph3d->SetMarkerStyle(kFullCircle);
            p_graph3d->SetTitle("Reconstructed data in XYZ coordinates; X "
                                "axis; Y axis; Z axis");
            loc_pad->cd(8);
            p_graph3d->Draw("P0");
            loc_pad->Modified();
            loc_pad->Update();

        } else {
            p_graph = nullptr;
            p_graph3d = nullptr;
        }

        loc_canv->Update();
    }

    loc_canv->WaitPrimitive();
}

/**
 * @brief Draws an UVW image of the entry from the .root file specified.
 *
 * @param filename The name of the .root file.
 * @param entry_nr The entry number to be converted.
 *
 * @return
 */
int drawUVWimage(TString filename = "./rootdata/data2.root",
                 TString outfolder =
                     "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/rawimages/",
                 int entry_nr = 0, bool opt_clean = false) {

    gROOT->SetBatch(kTRUE);

    generalDataStorage data_container;

    auto goodFile = filename;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();

    auto n_entries = good_data.returnNEntries();

    if (entry_nr >= n_entries) {

        return -1;
    }

    //###########################
    // DONT FORGET TO REMOVE THE TRUE
    err = good_data.decodeData(entry_nr, true);
    //##########################
    data_container.root_raw_data = good_data.returnRawData();

    convertUVW loc_conv_uvw(data_container.root_raw_data);

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return -2;

    err = loc_conv_uvw.makeConversion(opt_clean);
    if (err != 0)
        std::cout << "Make conversion error code " << err << std::endl;

    data_container.uvw_data = loc_conv_uvw.returnDataUVW();

    if (opt_clean) {
        /* cleanUVW loc_clean_uvw(data_container.uvw_data);

        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoU>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;

        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoV>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;

        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoW>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;

        data_container.uvw_data = loc_clean_uvw.returnDataUVW(); */
    }

    auto loc_canv = new TCanvas("uvw format", "UVW hists", 800, 600);
    loc_canv->SetFrameLineColor(0);
    loc_canv->SetFrameLineWidth(0);
    loc_canv->SetBottomMargin(0);
    loc_canv->SetTopMargin(0);
    loc_canv->SetLeftMargin(0);
    loc_canv->SetRightMargin(0);

    gStyle->SetOptStat(0);

    auto u_hists =
        new TH2D(Form("u_hists_%d", entry_nr), "", 512, 1, 513, 72, 1, 72);
    auto v_hists =
        new TH2D(Form("v_hists_%d", entry_nr), "", 512, 1, 513, 92, 1, 92);
    auto w_hists =
        new TH2D(Form("w_hists_%d", entry_nr), "", 512, 1, 513, 92, 1, 92);

    u_hists->SetTickLength(0);
    TAxis *u_axis = u_hists->GetYaxis();
    u_axis->SetTickLength(0);
    u_hists->SetLabelSize(0);
    u_axis->SetLabelSize(0);

    v_hists->SetTickLength(0);
    TAxis *v_axis = v_hists->GetYaxis();
    v_axis->SetTickLength(0);
    v_hists->SetLabelSize(0);
    v_axis->SetLabelSize(0);

    w_hists->SetTickLength(0);
    TAxis *w_axis = w_hists->GetYaxis();
    w_axis->SetTickLength(0);
    w_hists->SetLabelSize(0);
    w_axis->SetLabelSize(0);

    int bin = 0;
    // int strip = 1;

    for (const auto &iter : data_container.uvw_data) {

        bin = 0;

        if (iter.plane_val == 0) {

            for (const auto &sig_iter : iter.signal_val) {

                u_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 1) {

            for (const auto &sig_iter : iter.signal_val) {

                v_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 2) {

            for (const auto &sig_iter : iter.signal_val) {

                w_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }
    }

    u_hists->Draw("COLA");
    loc_canv->Update();
    loc_canv->Print(outfolder + Form("%d", entry_nr) + "_u" + ".png");

    v_hists->Draw("COLA");
    loc_canv->Update();
    loc_canv->Print(outfolder + Form("%d", entry_nr) + "_v" + ".png");

    w_hists->Draw("COLA");
    loc_canv->Update();
    loc_canv->Print(outfolder + Form("%d", entry_nr) + "_w" + ".png");

    loc_canv->Close();

    gROOT->SetBatch(kFALSE);

    return 0;
}

/**
 * @brief Hepls with the mass conversion to pdf. Uses the create_entries_pdf()
 * function.
 *
 * @param lin_arg The file to be converted, taken from the command line
 * argument.
 */
void mass_convertPDF(TString lin_arg) {

    TString dirandfileName = lin_arg;

    TString fileName = dirandfileName;

    TString dir = dirandfileName;
    int index = dir.Last('/');
    dir.Remove(index + 1, dir.Sizeof());

    index = fileName.Last('/');   // remove path

    fileName.Remove(0, index + 1);
    fileName.Remove(fileName.Sizeof() - 6, fileName.Sizeof());   // remove .root

    TString mkdirCommand = ".! mkdir ";
    mkdirCommand.Append(dir);
    mkdirCommand.Append("pdffiles");

    gROOT->ProcessLine(mkdirCommand);

    TString pdfName = dir + "pdffiles/" + fileName + ".pdf";

    create_entries_pdf(dirandfileName, pdfName, 50);
}

/**
 * @brief Hepls with the mass conversion to pdf. Uses the create_raw_pdf()
 * function.
 *
 * @param lin_arg The file to be converted, taken from the command line
 * argument.
 * @param nr_entries THe number of entries to be converted.
 */
void mass_convertRawPDF(TString lin_arg, int nr_entries = 10000) {

    TString dirandfileName = lin_arg;

    TString fileName = dirandfileName;

    TString dir = dirandfileName;
    int index = dir.Last('/');
    dir.Remove(index + 1, dir.Sizeof());

    index = fileName.Last('/');   // remove path

    fileName.Remove(0, index + 1);
    fileName.Remove(fileName.Sizeof() - 6, fileName.Sizeof());   // remove .root

    TString mkdirCommand = ".! mkdir ";
    mkdirCommand.Append(dir);
    mkdirCommand.Append("rawpdf");

    gROOT->ProcessLine(mkdirCommand);

    TString pdfName = dir + "rawpdf/" + fileName + ".pdf";

    create_raw_pdf(dirandfileName, pdfName, nr_entries);
}

void create_labeled_pdf(TString source_file, TString destination_file,
                        int read_entries) {

    int entry_nr = 0;
    int max_entries;

    auto goodFile = source_file;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    max_entries = good_data.returnNEntries();

    convertUVW loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    cleanUVW loc_clean_uvw;

    convertHitData loc_convert_hit;

    convertXYZ loc_conv_xyz;

    filterEventsXY loc_filter_xy;

    gROOT->SetBatch(kTRUE);

    auto loc_canv = new TCanvas("entries.pdf", "PDF", 1500, 1000);
    auto loc_pad = new TPad("pad name", "pad title", 0, 0, 1, 1);
    loc_pad->Divide(3, 3);
    loc_pad->Draw();

    TH2D *u_hists = nullptr;
    TH2D *v_hists = nullptr;
    TH2D *w_hists = nullptr;

    TGraph *u_graph = nullptr;
    TGraph *v_graph = nullptr;
    TGraph *w_graph = nullptr;

    TGraph *p_graph = nullptr;
    TGraph2D *p_graph3d = nullptr;

    loc_canv->Print(destination_file + "[");
    gErrorIgnoreLevel = kWarning;

    // only read the number of entries wanted
    max_entries = std::min(max_entries, read_entries);

    while (entry_nr < max_entries) {

        generalDataStorage data_container;

        std::cout << "\n\n\nNow at entry: " << entry_nr << " of " << max_entries
                  << " for " << destination_file << "\n";

        err = good_data.decodeData(entry_nr);
        if (err != 0) {
            std::cout << "Error decode data code " << err << "\n";
        }

        data_container.root_raw_data = good_data.returnRawData();

        std::cout << "RAW data size is " << data_container.root_raw_data.size()
                  << "\n";

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion();
        if (err != 0)
            std::cout << "Make conversion error code " << err << "\n";

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        std::cout << "UVW data size is " << data_container.uvw_data.size()
                  << "\n";

        /* reorderCh::reorderChfromFile<reorderCh::planeInfo_U>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_V>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_W>(
            data_container.uvw_data); */

        loc_clean_uvw.setUVWData(data_container.uvw_data);

        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoU>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoV>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoW>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;

        data_container.uvw_data = loc_clean_uvw.returnDataUVW();

        err = loc_convert_hit.setUVWData(data_container.uvw_data);
        if (err != 0)
            std::cout << "Error set UVW data code " << err << "\n";

        err = loc_convert_hit.getHitInfo();
        if (err != 0)
            std::cout << "Error get hit info code " << err << "\n";

        try {

            data_container.contains_vertical_line =
                loc_convert_hit.containsVerticalLine();

        } catch (std::exception &e) {

            std::cerr << "Exception detected at containsVerticalLine: \n\t"
                      << e.what() << std::endl;
        }

        data_container.hit_data = loc_convert_hit.returnHitData();
        data_container.raw_hist_container = loc_convert_hit.returnHistData();

        std::cout << "Hit data size " << data_container.hit_data.size() << "\n";
        std::cout << "Hist data size "
                  << data_container.raw_hist_container.size() << "\n";

        err = loc_conv_xyz.getNewVector(data_container.hit_data);
        if (err != 0)
            std::cout << "Error get new vector code " << err << "\n";

        err = loc_conv_xyz.makeConversionXYZ();
        if (err != 0)
            std::cout << "Error make conversion XYZ code " << err << "\n";

        data_container.xyz_data = loc_conv_xyz.returnXYZ();

        std::cout << "XYZ vector size " << data_container.xyz_data.size()
                  << "\n";

        data_container.n_entry = entry_nr;

        err = loc_filter_xy.filterAndPush(std::move(data_container));
        if (err != 0)
            std::cerr << "Error filter and push code " << err << "\n";

        entry_nr++;
    }

    // loc_filter_xy.assignClass();
    loc_filter_xy.assignClass_threaded();

    auto event_vec = loc_filter_xy.returnEventVector();

    loc_filter_xy.resetVector();

    for (const auto &curr_event : event_vec) {

        entry_nr = curr_event.n_entry;

        if (u_hists != nullptr)
            u_hists->Reset();

        if (v_hists != nullptr)
            v_hists->Reset();

        if (w_hists != nullptr)
            w_hists->Reset();

        if (u_graph != nullptr)
            u_graph->Delete();

        if (v_graph != nullptr)
            v_graph->Delete();

        if (w_graph != nullptr)
            w_graph->Delete();

        if (p_graph != nullptr)
            p_graph->Delete();

        if (p_graph3d != nullptr)
            p_graph3d->Delete();

        u_hists = new TH2D(Form("u_hists_%d", entry_nr),
                           Form("Histogram for U entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        v_hists = new TH2D(Form("v_hists_%d", entry_nr),
                           Form("Histogram for V entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        w_hists = new TH2D(Form("w_hists_%d", entry_nr),
                           Form("Histogram for W entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);

        int bin = 0;

        for (const auto &iter : curr_event.uvw_data) {

            bin = 0;

            if (iter.plane_val == 0) {

                for (const auto &sig_iter : iter.signal_val) {

                    u_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 1) {

                for (const auto &sig_iter : iter.signal_val) {

                    v_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 2) {

                for (const auto &sig_iter : iter.signal_val) {

                    w_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }
        }

        loc_pad->cd(1);
        u_hists->Draw("COLZ");
        loc_pad->cd(2);
        v_hists->Draw("COLZ");
        loc_pad->cd(3);
        w_hists->Draw("COLZ");

        std::vector<double> x_u;
        std::vector<double> y_u;
        std::vector<double> c_u;

        std::vector<double> x_v;
        std::vector<double> y_v;
        std::vector<double> c_v;

        std::vector<double> x_w;
        std::vector<double> y_w;
        std::vector<double> c_w;

        for (const auto &hit_iter : curr_event.hit_data) {

            if (hit_iter.plane == 0) {

                x_u.push_back(hit_iter.peak_x);
                y_u.push_back(hit_iter.strip);
                c_u.push_back(hit_iter.peak_y + hit_iter.base_line);

            } else if (hit_iter.plane == 1) {

                x_v.push_back(hit_iter.peak_x);
                y_v.push_back(hit_iter.strip);
                c_v.push_back(hit_iter.peak_y + hit_iter.base_line);

            } else if (hit_iter.plane == 2) {

                x_w.push_back(hit_iter.peak_x);
                y_w.push_back(hit_iter.strip);
                c_w.push_back(hit_iter.peak_y + hit_iter.base_line);
            }
        }

        if (curr_event.hit_data.size() != 0) {

            u_graph = new TGraph(x_u.size(), x_u.data(), y_u.data());
            u_graph->GetXaxis()->SetLimits(-1, 512);
            u_graph->GetHistogram()->SetMaximum(73);
            u_graph->GetHistogram()->SetMinimum(0);
            u_graph->SetMarkerColor(kBlack);
            u_graph->SetMarkerStyle(kFullCircle);
            u_graph->SetTitle("Hits detected on U plane; Time; Strip");

            // Set marker colors Pink > Red > Green > Blue
            /* if (c_u.size() != 0) {

                auto min_val_c_u = 0;
                auto max_val_c_u = 2000;

                std::transform(c_u.begin(), c_u.end(), c_u.begin(),
                               [min_val_c_u, max_val_c_u](const double x) {
                                   return (x - min_val_c_u) /
                                          (max_val_c_u - min_val_c_u);
                               });

                for (auto vec_i = 0; vec_i < c_u.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    u_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();

                    if (c_u[vec_i] < 0.33) {
                        TColor *loc_color = new TColor(0, 0, 3 * c_u[vec_i]);
                    } else if (c_u[vec_i] >= 0.33 && c_u[vec_i] < 0.66) {
                        TColor *loc_color =
                            new TColor(0, 3 * c_u[vec_i] / 2, 0);
                    } else if (c_u[vec_i] > 1) {
                        TColor *loc_color = new TColor(1, 0, 1);
                    } else {
                        TColor *loc_color = new TColor(c_u[vec_i], 0, 0);
                    }

                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    u_graph->GetListOfFunctions()->Add(loc_marker);
                }
            } */
            loc_pad->cd(4);
            u_graph->Draw("AP");

            v_graph = new TGraph(x_v.size(), x_v.data(), y_v.data());
            v_graph->GetXaxis()->SetLimits(-1, 512);
            v_graph->GetHistogram()->SetMaximum(93);
            v_graph->GetHistogram()->SetMinimum(0);
            v_graph->SetMarkerColor(kBlack);
            v_graph->SetMarkerStyle(kFullCircle);
            v_graph->SetTitle("Hits detected on V plane; Time; Strip");
            /* if (c_v.size() != 0) {

                auto min_val_c_v = 0;
                auto max_val_c_v = 2000;

                std::transform(c_v.begin(), c_v.end(), c_v.begin(),
                               [min_val_c_v, max_val_c_v](const double x) {
                                   return (x - min_val_c_v) /
                                          (max_val_c_v - min_val_c_v);
                               });

                for (auto vec_i = 0; vec_i < c_v.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    v_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();

                    if (c_v[vec_i] < 0.33) {
                        TColor *loc_color = new TColor(0, 0, 3 * c_v[vec_i]);
                    } else if (c_v[vec_i] >= 0.33 && c_v[vec_i] < 0.66) {
                        TColor *loc_color =
                            new TColor(0, 3 * c_v[vec_i] / 2, 0);
                    } else if (c_v[vec_i] > 1) {
                        TColor *loc_color = new TColor(1, 0, 1);
                    } else {
                        TColor *loc_color = new TColor(c_v[vec_i], 0, 0);
                    }

                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    v_graph->GetListOfFunctions()->Add(loc_marker);
                }
            } */
            loc_pad->cd(5);
            v_graph->Draw("AP");

            w_graph = new TGraph(x_w.size(), x_w.data(), y_w.data());
            w_graph->GetXaxis()->SetLimits(-1, 512);
            w_graph->GetHistogram()->SetMaximum(93);
            w_graph->GetHistogram()->SetMinimum(0);
            w_graph->SetMarkerColor(kBlack);
            w_graph->SetMarkerStyle(kFullCircle);
            w_graph->SetTitle("Hits detected on W plane; Time; Strip");
            /* if (c_w.size() != 0) {

                auto min_val_c_w = 0;
                auto max_val_c_w = 2000;

                std::transform(c_w.begin(), c_w.end(), c_w.begin(),
                               [min_val_c_w, max_val_c_w](const double x) {
                                   return (x - min_val_c_w) /
                                          (max_val_c_w - min_val_c_w);
                               });

                for (auto vec_i = 0; vec_i < c_w.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    w_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();

                    if (c_w[vec_i] < 0.33) {
                        TColor *loc_color = new TColor(0, 0, 3 * c_w[vec_i]);
                    } else if (c_w[vec_i] >= 0.33 && c_w[vec_i] < 0.66) {
                        TColor *loc_color =
                            new TColor(0, 3 * c_w[vec_i] / 2, 0);
                    } else if (c_w[vec_i] > 1) {
                        TColor *loc_color = new TColor(1, 0, 1);
                    } else {
                        TColor *loc_color = new TColor(c_w[vec_i], 0, 0);
                    }

                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    w_graph->GetListOfFunctions()->Add(loc_marker);
                }
            } */
            loc_pad->cd(6);
            w_graph->Draw("AP");

            loc_pad->Modified();
            loc_pad->Update();

        } else {
            u_graph = nullptr;
            v_graph = nullptr;
            w_graph = nullptr;
        }

        if (curr_event.xyz_data.size() != 0) {

            std::vector<double> x, y, z, chg;

            x.push_back(-10.0);
            y.push_back(-10.0);
            z.push_back(-10.0);
            chg.push_back(0.0);

            for (const auto &point_iter : curr_event.xyz_data) {

                x.push_back(point_iter.data_x);
                y.push_back(point_iter.data_y);
                z.push_back(point_iter.data_z);
                chg.push_back(point_iter.data_charge);
            }

            x.push_back(150.0);
            y.push_back(150.0);
            z.push_back(150.0);
            chg.push_back(0.0);

            p_graph = new TGraph(x.size(), x.data(), y.data());
            p_graph->GetXaxis()->SetLimits(-10, 150);
            p_graph->GetHistogram()->SetMaximum(150);
            p_graph->GetHistogram()->SetMinimum(-10);
            p_graph->SetMarkerStyle(kFullCircle);
            p_graph->SetTitle(Form(
                "XY coordinates projection with size %d and MSE %f; "
                "LABEL: %d; Straight line = %d; ",
                curr_event.xyz_data.size(), curr_event.mse_value,
                curr_event.filter_label, curr_event.contains_vertical_line));
            p_graph->SetMarkerColor(kBlack);

            /* if (chg.size() != 0) {

                auto min_val_chg = 0;
                auto max_val_chg = 6000;

                std::transform(chg.begin(), chg.end(), chg.begin(),
                               [min_val_chg, max_val_chg](const double x) {
                                   return (x - min_val_chg) /
                                          (max_val_chg - min_val_chg);
                               });

                for (auto vec_i = 0; vec_i < chg.size(); vec_i++) {

                    Double_t loc_x, loc_y;

                    p_graph->GetPoint(vec_i, loc_x, loc_y);

                    Int_t ci = TColor::GetFreeColorIndex();
                    TColor *loc_color = new TColor(ci, 0, 0, 0);

                    if (chg[vec_i] < 0.33) {
                        loc_color->SetRGB(0.0, 0.0, 3 * chg[vec_i]);
                    } else if (chg[vec_i] >= 0.33 && chg[vec_i] < 0.66) {
                        loc_color->SetRGB(0.0, 3 * chg[vec_i] / 2, 0.0);
                    } else if (chg[vec_i] > 1) {
                        loc_color->SetRGB(1.0, 0.0, 1.0);
                    } else {
                        loc_color->SetRGB(chg[vec_i], 0.0, 0.0);
                    }


                    TMarker *loc_marker = new TMarker(loc_x, loc_y, 20);
                    loc_marker->SetMarkerColor(ci);
                    p_graph->GetListOfFunctions()->Add(loc_marker);
                }
            } */
            loc_pad->cd(7);
            p_graph->Draw("AP");
            loc_pad->Modified();
            loc_pad->Update();

            p_graph3d = new TGraph2D(x.size(), x.data(), y.data(), z.data());
            p_graph3d->SetMarkerColor(kBlue);
            p_graph3d->SetMarkerStyle(kFullCircle);
            p_graph3d->SetTitle("Reconstructed data in XYZ coordinates; X "
                                "axis; Y axis; Z axis");
            loc_pad->cd(8);
            p_graph3d->Draw("P0");
            loc_pad->Modified();
            loc_pad->Update();

        } else {
            p_graph = nullptr;
            p_graph3d = nullptr;
        }

        std::cout << "Written entry " << entry_nr << "\n\n";

        loc_pad->Update();
        loc_canv->Update();
        loc_canv->Print(destination_file);
    }

    gErrorIgnoreLevel = kPrint;
    loc_canv->Print(destination_file + "]");

    loc_canv->Close();

    if (loc_canv) {
        delete (loc_canv);
        loc_canv = nullptr;
    }

    gROOT->SetBatch(kFALSE);

    std::cout << "\n\n\nDONE!!!!!\n\n\n" << std::endl;
}

void createLabeledXYZcsv(TString source_file, TString destination_file,
                         int label = 0) {

    int entry_nr = 0;
    int max_entries;

    auto goodFile = source_file;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    max_entries = good_data.returnNEntries();

    convertUVW loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    cleanUVW loc_clean_uvw;

    convertHitData loc_convert_hit;

    convertXYZ loc_conv_xyz;

    filterEventsXY loc_filter_xy;

    while (entry_nr < max_entries) {

        generalDataStorage data_container;

        std::cout << "\n\n\nNow at entry: " << entry_nr << " of " << max_entries
                  << " for " << destination_file << "\n";

        err = good_data.decodeData(entry_nr);
        if (err != 0) {
            std::cout << "Error decode data code " << err << "\n";
        }

        data_container.root_raw_data = good_data.returnRawData();

        std::cout << "RAW data size is " << data_container.root_raw_data.size()
                  << "\n";

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion();
        if (err != 0)
            std::cout << "Make conversion error code " << err << "\n";

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        std::cout << "UVW data size is " << data_container.uvw_data.size()
                  << "\n";

        /* reorderCh::reorderChfromFile<reorderCh::planeInfo_U>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_V>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_W>(
            data_container.uvw_data); */

        loc_clean_uvw.setUVWData(data_container.uvw_data);

        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoU>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoV>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoW>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;

        data_container.uvw_data = loc_clean_uvw.returnDataUVW();

        std::cout << "Cleaned data size: " << data_container.uvw_data.size()
                  << "\n";

        err = loc_convert_hit.setUVWData(data_container.uvw_data);
        if (err != 0)
            std::cout << "Error set UVW data code " << err << "\n";

        err = loc_convert_hit.getHitInfo();
        if (err != 0)
            std::cout << "Error get hit info code " << err << "\n";

        try {
            if (loc_convert_hit.containsVerticalLine() == true) {

                std::cout << "\n\nContains vertical line!\n\n";
            }
        } catch (std::exception &e) {

            std::cerr << "Exception detected at containsVerticalLine: \n\t"
                      << e.what() << std::endl;
        }

        data_container.hit_data = loc_convert_hit.returnHitData();
        // data_container.raw_hist_container = loc_convert_hit.returnHistData();

        std::cout << "Hit data size " << data_container.hit_data.size() << "\n";
        /* std::cout << "Hist data size "
                  << data_container.raw_hist_container.size() << "\n"; */

        err = loc_conv_xyz.getNewVector(data_container.hit_data);
        if (err != 0)
            std::cout << "Error get new vector code " << err << "\n";

        err = loc_conv_xyz.makeConversionXYZ();
        if (err != 0)
            std::cout << "Error make conversion XYZ code " << err << "\n";

        data_container.xyz_data = loc_conv_xyz.returnXYZ();

        std::cout << "XYZ vector size " << data_container.xyz_data.size()
                  << "\n";

        data_container.n_entry = entry_nr;

        err = loc_filter_xy.filterAndPush(std::move(data_container));
        if (err != 0)
            std::cerr << "Error filter and push code " << err << "\n";

        entry_nr++;
    }

    std::cout << "\n\nBeginning to assign class." << std::endl;

    // loc_filter_xy.assignClass();
    loc_filter_xy.assignClass_threaded();

    auto event_vec = loc_filter_xy.returnEventVector();

    std::cout << "Event vector size is " << event_vec.size() << std::endl;

    std::ofstream out_file(destination_file);

    if (!out_file.is_open()) {
        std::cerr << "Error: Could not open file for output" << std::endl;
        return;
    }

    // header
    out_file << "x,y,z,entry_nr,label\n";

    for (const auto &event : event_vec) {

        // For label 0 we take all events. Else we only print the events with
        // the label passed to the function.
        if (event.filter_label != label && label != 0)
            continue;

        for (const auto &data_entry : event.xyz_data) {

            out_file << data_entry.data_x << "," << data_entry.data_y << ","
                     << data_entry.data_z << "," << event.n_entry << ","
                     << event.filter_label;

            out_file << "\n";
        }
    }

    out_file.close();

    std::cout << "\n\n\nDONE!!!!!!!\n\n\n" << std::endl;
}

void mass_convertLabeledPDF(TString lin_arg, int nr_entries = 10000) {

    TString dirandfileName = lin_arg;

    TString fileName = dirandfileName;

    TString dir = dirandfileName;
    int index = dir.Last('/');
    dir.Remove(index + 1, dir.Sizeof());

    index = fileName.Last('/');   // remove path

    fileName.Remove(0, index + 1);
    fileName.Remove(fileName.Sizeof() - 6, fileName.Sizeof());   // remove .root

    TString mkdirCommand = ".! mkdir ";
    mkdirCommand.Append(dir);
    mkdirCommand.Append("labeledpdf");

    gROOT->ProcessLine(mkdirCommand);

    TString pdfName = dir + "labeledpdf/" + fileName + ".pdf";

    create_labeled_pdf(dirandfileName, pdfName, nr_entries);
}

std::tuple<std::vector<generalDataStorage>, int>
returnLabeledData(TString source_file) {
    int entry_nr = 0;
    int max_entries;

    auto goodFile = source_file;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    max_entries = good_data.returnNEntries();

    convertUVW loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return {std::vector<generalDataStorage>(), 0};

    cleanUVW loc_clean_uvw;

    convertHitData loc_convert_hit;

    convertXYZ loc_conv_xyz;

    filterEventsXY loc_filter_xy;

    while (entry_nr < max_entries) {

        generalDataStorage data_container;

        std::cout << "\n\n\nNow at entry: " << entry_nr << " of " << max_entries
                  << " for " << source_file << "\n";

        err = good_data.decodeData(entry_nr);
        if (err != 0) {
            std::cout << "Error decode data code " << err << "\n";
        }

        data_container.root_raw_data = good_data.returnRawData();

        std::cout << "RAW data size is " << data_container.root_raw_data.size()
                  << "\n";

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion();
        if (err != 0)
            std::cout << "Make conversion error code " << err << "\n";

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        std::cout << "UVW data size is " << data_container.uvw_data.size()
                  << "\n";

        std::vector<rawData>().swap(data_container.root_raw_data);

        /* reorderCh::reorderChfromFile<reorderCh::planeInfo_U>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_V>(
            data_container.uvw_data);
        reorderCh::reorderChfromFile<reorderCh::planeInfo_W>(
            data_container.uvw_data); */

        loc_clean_uvw.setUVWData(data_container.uvw_data);

        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoU>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoV>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoW>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;

        data_container.uvw_data = loc_clean_uvw.returnDataUVW();

        err = loc_convert_hit.setUVWData(data_container.uvw_data);
        if (err != 0)
            std::cout << "Error set UVW data code " << err << "\n";

        err = loc_convert_hit.getHitInfo();
        if (err != 0)
            std::cout << "Error get hit info code " << err << "\n";

        data_container.hit_data = loc_convert_hit.returnHitData();
        data_container.raw_hist_container = loc_convert_hit.returnHistData();

        std::cout << "Hit data size " << data_container.hit_data.size() << "\n";
        std::cout << "Hist data size "
                  << data_container.raw_hist_container.size() << "\n";

        std::vector<dataUVW>().swap(data_container.uvw_data);

        err = loc_conv_xyz.getNewVector(data_container.hit_data);
        if (err != 0)
            std::cout << "Error get new vector code " << err << "\n";

        err = loc_conv_xyz.makeConversionXYZ();
        if (err != 0)
            std::cout << "Error make conversion XYZ code " << err << "\n";

        data_container.xyz_data = loc_conv_xyz.returnXYZ();

        std::cout << "XYZ vector size " << data_container.xyz_data.size()
                  << "\n";

        data_container.n_entry = entry_nr;

        err = loc_filter_xy.filterAndPush(std::move(data_container));
        if (err != 0)
            std::cerr << "Error filter and push code " << err << "\n";

        entry_nr++;
    }

    // loc_filter_xy.assignClass();
    loc_filter_xy.assignClass_threaded();

    auto event_vec = loc_filter_xy.returnEventVector();

    return {std::move(event_vec), max_entries};
}

void countUsefulEvents(TString file_path) {

    // 0 is the number of entries, 1 and 2 are the number of respective labels
    std::map<int, int> label_count{{0, 0}, {1, 0}, {2, 0}};

    std::vector<generalDataStorage> event_vec;

    int nr_entries = 0;

    glob_t glob_result;
    glob(file_path, GLOB_TILDE, NULL, &glob_result);
    for (unsigned int i = 0; i < glob_result.gl_pathc; ++i) {

        std::string path = glob_result.gl_pathv[i];
        if (path.length() < 5 || path.substr(path.length() - 5) != ".root") {
            continue;
        }

        nr_entries = 0;

        std::tie(event_vec, nr_entries) = returnLabeledData(path);

        if (event_vec.size() == 0) {

            std::cerr << "Event vector size is 0 for file " << path
                      << std::endl;
            continue;
        }

        for (const auto &event : event_vec) {

            label_count[event.filter_label]++;
        }

        label_count[0] += nr_entries;

        std::vector<generalDataStorage>().swap(event_vec);
    }

    std::ofstream out_file("./converteddata/counts.csv");

    if (!out_file.is_open()) {
        std::cerr << "Error: Could not open file for output" << std::endl;
        return;
    }

    // header
    out_file << "Number_of_entries,Label_1_count,Label_2_count\n"
             << label_count[0] << "," << label_count[1] << "," << label_count[2]
             << "\n";

    out_file.close();
}

void countEventsFromFile(TString filename) {

    std::ofstream out_file("./converteddata/counts_each.csv",
                           std::ios_base::app);

    if (!out_file.is_open()) {
        std::cerr << "Error: Could not open file for output" << std::endl;
        return;
    }

    // 0 is the number of entries, 1 and 2 are the number of respective labels
    std::map<int, int> label_count{{0, 0}, {1, 0}, {2, 0}};

    std::vector<generalDataStorage> event_vec;

    int nr_entries = 0;

    std::tie(event_vec, nr_entries) = returnLabeledData(filename);

    for (const auto &event : event_vec) {

        label_count[event.filter_label]++;

        if (event.filter_label == 1)
            std::cout << "Label " << event.filter_label << " current count "
                      << label_count[event.filter_label] << std::endl;
    }

    label_count[0] += nr_entries;

    out_file << filename << "," << label_count[0] << "," << label_count[1]
             << "," << label_count[2] << "\n";

    out_file.close();
}

void createUVWcsv(TString source_file, TString destination_file) {

    int entry_nr = 0;
    int max_entries;

    auto goodFile = source_file;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    max_entries = good_data.returnNEntries();

    convertUVW loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    cleanUVW loc_clean_uvw;

    convertHitData loc_convert_hit;

    std::ofstream out_file(destination_file);

    if (!out_file.is_open()) {
        std::cerr << "Error: Could not open file for output" << std::endl;
        return;
    }

    // header
    out_file << "entry_nr,plane,strip,time_bin\n";

    while (entry_nr < max_entries) {

        generalDataStorage data_container;

        std::cout << "\n\n\nNow at entry: " << entry_nr << " of " << max_entries
                  << " for " << destination_file << "\n";

        err = good_data.decodeData(entry_nr);
        if (err != 0) {
            std::cout << "Error decode data code " << err << "\n";
        }

        data_container.root_raw_data = good_data.returnRawData();

        std::cout << "RAW data size is " << data_container.root_raw_data.size()
                  << "\n";

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion();
        if (err != 0)
            std::cout << "Make conversion error code " << err << "\n";

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        std::cout << "UVW data size is " << data_container.uvw_data.size()
                  << "\n";

        loc_clean_uvw.setUVWData(data_container.uvw_data);

        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoU>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoV>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;
        err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoW>();
        if (err != 0)
            std::cerr << "Error substractBl code " << err << std::endl;

        data_container.uvw_data = loc_clean_uvw.returnDataUVW();

        std::cout << "Cleaned data size: " << data_container.uvw_data.size()
                  << "\n";

        err = loc_convert_hit.setUVWData(data_container.uvw_data);
        if (err != 0)
            std::cout << "Error set UVW data code " << err << "\n";

        err = loc_convert_hit.getHitInfo();
        if (err != 0)
            std::cout << "Error get hit info code " << err << "\n";

        data_container.hit_data = loc_convert_hit.returnHitData();
        data_container.raw_hist_container = loc_convert_hit.returnHistData();

        std::cout << "Hit data size " << data_container.hit_data.size() << "\n";

        std::sort(data_container.hit_data.begin(),
                  data_container.hit_data.end(),
                  [](const hitPoints &a, const hitPoints &b) {
                      return a.plane < b.plane;
                  });

        for (const auto &hit : data_container.hit_data) {

            out_file << entry_nr << "," << hit.plane << "," << hit.strip << ","
                     << hit.peak_x << "\n";
        }

        entry_nr++;
    }

    out_file.close();

    std::cout << "\n\n\nDONE!!!!!!!\n\n\n" << std::endl;
}

void mass_create_raw_images(TString lin_arg, int nr_entries = 10000) {
    TString dirandfileName = lin_arg;

    TString fileName = dirandfileName;

    TString dir = dirandfileName;
    int index = dir.Last('/');
    dir.Remove(index + 1, dir.Sizeof());

    index = fileName.Last('/');   // remove path

    fileName.Remove(0, index + 1);
    fileName.Remove(fileName.Sizeof() - 6, fileName.Sizeof());   // remove .root

    TString mkdirCommand = ".! mkdir ";
    mkdirCommand.Append(dir);
    mkdirCommand.Append("rawimages");

    gROOT->ProcessLine(mkdirCommand);

    mkdirCommand.Append("/");
    mkdirCommand.Append(fileName);

    gROOT->ProcessLine(mkdirCommand);

    TString imagePath = dir + "rawimages/" + fileName + "/";

    for (auto i = 0; i < nr_entries; i++) {

        if (drawUVWimage(dirandfileName, imagePath, i) != 0) {

            break;
        }
    }
}

void mass_create_clean_images(TString lin_arg, bool zip_opt = false,
                              int nr_entries = 10000) {
    TString dirandfileName = lin_arg;

    TString fileName = dirandfileName;

    TString dir = dirandfileName;
    int index = dir.Last('/');
    dir.Remove(index + 1, dir.Sizeof());

    index = fileName.Last('/');   // remove path

    fileName.Remove(0, index + 1);
    fileName.Remove(fileName.Sizeof() - 6, fileName.Sizeof());   // remove .root

    TString mkdirCommand = ".! mkdir ";
    mkdirCommand.Append(dir);
    mkdirCommand.Append("cleanimages");

    gROOT->ProcessLine(mkdirCommand);

    mkdirCommand.Append("/");
    mkdirCommand.Append(
        fileName);   // Create folder with the name of the root file.

    gROOT->ProcessLine(mkdirCommand);

    TString imagePath = dir + "cleanimages/" + fileName + "/";

    for (auto i = 0; i < nr_entries; i++) {

        if (drawUVWimage(dirandfileName, imagePath, i, true) != 0) {

            break;
        }
    }

    if (zip_opt) {

        TString zipCommand = ".! zip -j -r -m " + dir + "cleanimages/" +
                             fileName + ".zip " + imagePath;

        gROOT->ProcessLine(zipCommand);

        TString rmCommand = ".! rm -r " + imagePath;

        gROOT->ProcessLine(rmCommand);
    }
}

// root -q "runmacro.cpp(\"a\")"
void runmacro(TString lin_arg) {

    /* view_data_entries("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                      "CoBo_2018-06-20T10-51-39.459_0000.root"); */

    /* view_data_entries("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                      "CoBo_2018-06-16T10-18-38.616_0000.root"); */

    /* view_data_entries("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                      "CoBo_2018-06-20T10-51-39.459_0002.root"); */

    /* view_raw_data("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                  "CoBo_2018-06-16T10-18-38.616_0000.root",
                  1); */

    /* create_entries_pdf("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                       "CoBo_2018-06-20T10-51-39.459_0000.root",
                       "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/finalpdf/"
                       "CoBo_2018-06-20T10-51-39.459_0000.pdf",
                       10000, 1); */

    /* create_labeled_pdf(
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
        "CoBo_2018-06-20T10-51-39.459_0000.root",
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/labeledpdf/"
        "CoBo_2018-06-20T10-51-39.459_0000.pdf",
        10000); */

    /* create_labeled_pdf(
        "./rootdata/data2.root",
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/labeledpdf/"
        "data2.pdf",
        10000, 1); */

    /* create_raw_pdf("./rootdata/data2.root",
                   "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/rawpdf/"
                   "data2.pdf",
                   10000); */

    // writeXYZcvs(429);

    // drawXYimage(429);

    // writeFullXYZCSV();

    /* drawUVWimage("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                 "CoBo_2018-06-20T10-51-39.459_0000.root",
                 "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/rawimages/"
                 "CoBo_2018-06-20T10-51-39.459_0000/",
                 25); */

    // mass_convertPDF(lin_arg);

    // mass_convertRawPDF(lin_arg, 10000);

    // createNormCSV(false);

    // printPeaksByChannel("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/CoBo_2018-06-20T10-35-30.853_0005.root",
    // 424, 0, 1);

    /* createLabeledXYZcsv(
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
        "CoBo_2018-06-20T10-51-39.459_0005.root",
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/labeledcsv/"
        "CoBo_2018-06-20T10-51-39.459_0005.csv",
        0); */

    /* createLabeledXYZcsv(
        "./rootdata/data2.root",
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/labeledcsv/"
        "data2.csv",
        0); */

    /* createUVWcsv("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                 "CoBo_2018-06-16T10-18-38.616_0000.root",
                 "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/uvwcsv/"
                 "CoBo_2018-06-16T10-18-38.616_0000.csv"); */

    /* createUVWcsv("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                 "CoBo_2018-06-20T10-51-39.459_0005.root",
                 "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/uvwcsv/"
                 "CoBo_2018-06-20T10-51-39.459_0005.csv");
 */
    // mass_convertLabeledPDF(lin_arg, 10000);

    // countUsefulEvents("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/*");

    // countEventsFromFile(lin_arg);

    /* mass_create_raw_images("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                           "CoBo_2018-06-20T10-51-39.459_0000.root"); */

    /* mass_create_clean_images("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                             "CoBo_2018-06-20T13-12-47.629_0002.root",
                             true); */

    mass_create_clean_images(lin_arg, true);
}