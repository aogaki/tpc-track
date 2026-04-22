//runmacro_mini({"-view","-norm1","-clean1","-zip0","CoBo_2025-09-04T09_36_04.527_0000.root"})
#include <cstdio>
#include <glob.h>
#include <iostream>
#include <limits>
#include <map>
#include <random>

#include "TString.h"
#include "include/ErrorCodesMap.hpp"
#include "src/cleanUVW.cpp"
#include "src/convertUVW_mini.cpp"
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

void viewUVWdata_mini(TString fileName, bool opt_norm = true) {

    int entry_nr = -1;
    int max_entries;

    auto goodFile = fileName;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    max_entries = good_data.returnNEntries();

    convertUVW_mini loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    cleanUVW loc_clean_uvw;

    auto loc_canv =
        new TCanvas("View entries canvas", "View entries", 1500, 1000);
    auto loc_pad = new TPad("pad name", "pad title", 0, 0, 1, 1);
    loc_pad->Divide(4, 2);
    loc_pad->Draw();

    TH2D *u_hists = nullptr;
    TH2D *v_hists = nullptr;
    TH2D *w_hists = nullptr;
    TH1D *c_hist = nullptr;

    TH2D *u_hists_clean = nullptr;
    TH2D *v_hists_clean = nullptr;
    TH2D *w_hists_clean = nullptr;
    TH1D *c_hist_clean = nullptr;

    uint32_t loop_nr = 0;

    while (1) {

        generalDataStorage data_container;

        generalDataStorage data_container_clean;

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

            if (entry_nr < (max_entries - 1)) {
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

        err = good_data.decodeData(entry_nr, true);
        if (err != 0) {
            std::cout << "Error decode data code " << err << std::endl;
        }

        data_container.root_raw_data = good_data.returnRawData();

        std::cout << "RAW data size is " << data_container.root_raw_data.size()
                  << std::endl;

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion(opt_norm, false);
        if (err != 0)
            std::cout << "Make conversion error code " << err << std::endl;

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        std::cout << "UVW data size is " << data_container.uvw_data.size()
                  << std::endl;

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

        data_container_clean.uvw_data = loc_clean_uvw.returnDataUVW();

        // Reset hists

        if (u_hists != nullptr)
            u_hists->Reset();

        if (v_hists != nullptr)
            v_hists->Reset();

        if (w_hists != nullptr)
            w_hists->Reset();

        if (c_hist != nullptr)
            c_hist->Reset();

        if (u_hists_clean != nullptr)
            u_hists_clean->Reset();

        if (v_hists_clean != nullptr)
            v_hists_clean->Reset();

        if (w_hists_clean != nullptr)
            w_hists_clean->Reset();

        if (c_hist_clean != nullptr)
            c_hist_clean->Reset();

        // Crete new hists

        u_hists = new TH2D(Form("u_hists_%d", entry_nr),
                           Form("Histogram for U entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        v_hists = new TH2D(Form("v_hists_%d", entry_nr),
                           Form("Histogram for V entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);
        w_hists = new TH2D(Form("w_hists_%d", entry_nr),
                           Form("Histogram for W entry %d", entry_nr), 512, 1,
                           513, 100, 1, 101);

        c_hist = new TH1D(Form("c_hist_%d", entry_nr),
                          Form("Histogram for charge entry %d", entry_nr), 512,
                          1, 513);

        u_hists_clean =
            new TH2D(Form("u_hists_clean_%d", entry_nr),
                     Form("Clean histogram for U entry %d", entry_nr), 512, 1,
                     513, 100, 1, 101);
        v_hists_clean =
            new TH2D(Form("v_hists_clean_%d", entry_nr),
                     Form("Clean histogram for V entry %d", entry_nr), 512, 1,
                     513, 100, 1, 101);
        w_hists_clean =
            new TH2D(Form("w_hists_clean_%d", entry_nr),
                     Form("Clean histogram for W entry %d", entry_nr), 512, 1,
                     513, 100, 1, 101);

        c_hist_clean = new TH1D(
            Form("c_hist_clean_%d", entry_nr),
            Form("Clean istogram for charge entry %d", entry_nr), 512, 1, 513);

        // Set the color scale

        // gStyle->SetPalette(kGreyScale);

        /* u_hists->SetMinimum(-100);
        u_hists->SetMaximum(1000);

        v_hists->SetMinimum(-100);
        v_hists->SetMaximum(1000);

        w_hists->SetMinimum(-100);
        w_hists->SetMaximum(1000);

        u_hists_clean->SetMinimum(-100);
        u_hists_clean->SetMaximum(1000);

        v_hists_clean->SetMinimum(-100);
        v_hists_clean->SetMaximum(1000);

        w_hists_clean->SetMinimum(-100);
        w_hists_clean->SetMaximum(1000); */

        // Fill the raw hists

        int bin = 0;

        std::array<double, 512> ch_array = {0};

        for (const auto &iter : data_container.uvw_data) {

            // All signals should have size 512;
            if (iter.signal_val.size() == 512) {

                for (auto i = 0; i < 512; ++i) {

                    ch_array[i] += iter.signal_val[i];
                }
            }

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

        // Clean hists

        bin = 0;

        std::array<double, 512> ch_array_clean = {0};

        for (const auto &iter : data_container_clean.uvw_data) {

            // All signals should have size 512;
            if (iter.signal_val.size() == 512) {

                for (auto i = 0; i < 512; ++i) {

                    ch_array_clean[i] += iter.signal_val[i];
                }
            }

            bin = 0;

            if (iter.plane_val == 0) {

                for (const auto &sig_iter : iter.signal_val) {

                    u_hists_clean->SetBinContent(++bin, iter.strip_nr,
                                                 sig_iter);
                }
            }

            if (iter.plane_val == 1) {

                for (const auto &sig_iter : iter.signal_val) {

                    v_hists_clean->SetBinContent(++bin, iter.strip_nr,
                                                 sig_iter);
                }
            }

            if (iter.plane_val == 2) {

                for (const auto &sig_iter : iter.signal_val) {

                    w_hists_clean->SetBinContent(++bin, iter.strip_nr,
                                                 sig_iter);
                }
            }
        }

        // Fill the charge hists.

        bin = 0;

        for (const auto &ch_el_iter : ch_array) {

            c_hist->SetBinContent(++bin, ch_el_iter);
        }

        bin = 0;

        for (const auto &ch_el_iter : ch_array_clean) {

            c_hist_clean->SetBinContent(++bin, ch_el_iter);
        }

        // Draw the hists.

        loc_pad->cd(1);
        u_hists->Draw("COLZ");
        loc_pad->cd(2);
        v_hists->Draw("COLZ");
        loc_pad->cd(3);
        w_hists->Draw("COLZ");   // CONT4Z or COLZ
        loc_pad->cd(4);
        c_hist->Draw();

        loc_pad->cd(5);
        u_hists_clean->Draw("COLZ");
        loc_pad->cd(6);
        v_hists_clean->Draw("COLZ");
        loc_pad->cd(7);
        w_hists_clean->Draw("COLZ");
        loc_pad->cd(8);
        c_hist_clean->Draw();

        loc_canv->Update();
    }

    loc_canv->WaitPrimitive();
}

int drawUVWimage_mini(
    TString filename = "./rootdata/data2.root",
    TString outfolder =
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/rawimages/",
    int entry_nr = 0, bool opt_norm = false, bool opt_clean = false) {

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
    // DONT FORGET TO REMOVE THE TRUE FOR NO FPN
    err = good_data.decodeData(entry_nr, true);
    //##########################
    data_container.root_raw_data = good_data.returnRawData();

    convertUVW_mini loc_conv_uvw;

    loc_conv_uvw.setRawData(data_container.root_raw_data);

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return -2;

    err = loc_conv_uvw.makeConversion(opt_norm, false);
    if (err != 0)
        std::cout << "Make conversion error code " << err << std::endl;

    data_container.uvw_data = loc_conv_uvw.returnDataUVW();

    if (opt_clean) {
        cleanUVW loc_clean_uvw;

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
        new TH2D(Form("u_hists_%d", entry_nr), "", 512, 1, 512, 72, 1, 72);
    auto v_hists =
        new TH2D(Form("v_hists_%d", entry_nr), "", 512, 1, 512, 92, 1, 92);
    auto w_hists =
        new TH2D(Form("w_hists_%d", entry_nr), "", 512, 1, 512, 92, 1, 92);

    // gStyle->SetPalette(kGreyScale);

    u_hists->SetMinimum(-1);
    u_hists->SetMaximum(1000);

    v_hists->SetMinimum(-1);
    v_hists->SetMaximum(1000);

    w_hists->SetMinimum(-1);
    w_hists->SetMaximum(1000);

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

    u_hists->Draw("COL");
    loc_canv->SetWindowSize(512, (3 * 72));
    loc_canv->Update();
    loc_canv->Print(outfolder + Form("%d", entry_nr) + "_u" + ".png");

    v_hists->Draw("COL");
    loc_canv->SetWindowSize(512, (3 * 92));
    loc_canv->Update();
    loc_canv->Print(outfolder + Form("%d", entry_nr) + "_v" + ".png");

    w_hists->Draw("COL");
    loc_canv->SetWindowSize(512, (3 * 92));
    loc_canv->Update();
    loc_canv->Print(outfolder + Form("%d", entry_nr) + "_w" + ".png");

    loc_canv->Close();

    gROOT->SetBatch(kFALSE);

    return 0;
}

void mass_create_clean_images_mini(TString lin_arg, bool zip_opt = false,
                                   bool opt_norm = true, bool opt_clean = false,
                                   int nr_entries = 10000) {
    TString dirandfileName = lin_arg;

    TString out_dir_name = "images_default";

    if (opt_clean && opt_norm) {

        out_dir_name = "images_aggressive";
    } else if (!opt_clean && opt_norm) {

        out_dir_name = "images_mild";
    } else if (!opt_clean && !opt_norm) {

        out_dir_name = "images_none";
    }

    TString fileName = dirandfileName;

    TString dir = dirandfileName;
    int index = dir.Last('/');
    dir.Remove(index + 1, dir.Sizeof());

    index = fileName.Last('/');   // remove path

    fileName.Remove(0, index + 1);
    fileName.Remove(fileName.Sizeof() - 6, fileName.Sizeof());   // remove .root

    TString mkdirCommand = ".! mkdir ";
    mkdirCommand.Append(dir);
    mkdirCommand.Append(out_dir_name);

    gROOT->ProcessLine(mkdirCommand);

    mkdirCommand.Append("/");
    mkdirCommand.Append(
        fileName);   // Create folder with the name of the root file.

    gROOT->ProcessLine(mkdirCommand);

    TString imagePath = dir + out_dir_name + "/" + fileName + "/";

    for (auto i = 0; i < nr_entries; i++) {

        if (drawUVWimage_mini(dirandfileName, imagePath, i, opt_norm,
                              opt_clean) != 0) {

            break;
        }
    }

    if (zip_opt) {

        TString zipCommand = ".! zip -j -r -m " + dir + out_dir_name + "/" +
                             fileName + ".zip " + imagePath;

        gROOT->ProcessLine(zipCommand);

        TString rmCommand = ".! rm -r " + imagePath;

        gROOT->ProcessLine(rmCommand);
    }
}

// root -q "runmacro_mini.cpp(\"a\")"
void runmacro_mini(std::initializer_list<TString> lin_arg) {

    /* viewUVWdata_mini("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                     "CoBo_2018-06-20T10-51-39.459_0000.root"); */

    /* mass_create_clean_images_mini(
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
        "CoBo_2018-06-20T10-51-39.459_0000.root");

    mass_create_clean_images_mini(
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
        "CoBo_2018-06-20T10-51-39.459_0002.root");

    mass_create_clean_images_mini(
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
        "CoBo_2018-06-20T10-51-39.459_0003.root");

    mass_create_clean_images_mini(
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
        "CoBo_2018-06-20T10-51-39.459_0004.root");

    mass_create_clean_images_mini(
        "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
        "CoBo_2018-06-20T10-51-39.459_0005.root"); */

    if (lin_arg.size() == 0) {

        std::cerr << "Error: The command line argument cannot be empty.";

        return;
    }

    auto lin_arg_it = lin_arg.begin();

    // First argument must be the chosen function.
    auto function_option = (*lin_arg_it);

    // Second argument is the normalization option.
    ++lin_arg_it;

    bool norm_option = false;

    if ((*lin_arg_it) == "-norm0") {

        norm_option = false;
    } else if ((*lin_arg_it) == "-norm1") {

        norm_option = true;
    } else {

        std::cerr << "Error: Invalid normalization option.";
        return;
    }

    // Third argument is the clean option.
    ++lin_arg_it;

    bool clean_option = false;

    if ((*lin_arg_it) == "-clean0") {

        clean_option = false;
    } else if ((*lin_arg_it) == "-clean1") {

        clean_option = true;
    } else {

        std::cerr << "Error: Invalid clean option.";
        return;
    }

    // Fourth argument is the clean option.
    ++lin_arg_it;

    bool zip_option = false;

    if ((*lin_arg_it) == "-zip0") {

        zip_option = false;
    } else if ((*lin_arg_it) == "-zip1") {

        zip_option = true;
    } else {

        std::cerr << "Error: Invalid zip option.";
        return;
    }

    ++lin_arg_it;

    std::cout << "Running with options: " << norm_option << " " << clean_option
              << " " << zip_option << std::endl;

    // The last arguments should be the files.

    if (function_option == "-view") {

        viewUVWdata_mini((*lin_arg_it), norm_option);
    } else if (function_option == "-convert") {

        mass_create_clean_images_mini((*lin_arg_it), zip_option, norm_option,
                                      clean_option);
    } else {

        std::cerr << "Error: Invalid view/convert option.";
        return;
    }
}
