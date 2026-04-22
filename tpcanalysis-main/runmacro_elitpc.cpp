#include <cstdio>
#include <glob.h>
#include <iostream>
#include <limits>
#include <map>
#include <random>

#include "TString.h"
#include "include/ErrorCodesMap.hpp"
#include "src/cleanUVW.cpp"
#include "src/convertUVW_elitpc.cpp"
#include "src/loadData.cpp"
#include "src/miscFunc_elitpc.cpp"

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

/// @brief
/// @param fileName
/// @param opt_norm
/// @param opt_clean
void viewUVWdata_elitpc(std::vector<TString> fileName, bool opt_norm = false,
                        bool opt_clean = false) {

    std::vector<generalDataStorage> data_vec;

    // ELITPC CoBo files usually come in groups of 4, one for each AsAd, so we
    // have to combine them.
    for (const auto &file_entry : fileName) {
        try {

            std::cout << "File " << file_entry << std::endl;

            // data_vec = miscFunc_elitpc::getAllEntries(file_entry, true,
            // true);
            std::vector<generalDataStorage> temp_data =
                miscFunc_elitpc::getAllEntries(file_entry, true, opt_norm);
            data_vec.insert(data_vec.end(),
                            std::make_move_iterator(temp_data.begin()),
                            std::make_move_iterator(temp_data.end()));
        } catch (const std::exception &e) {

            std::cout << "Encountered error when getting all entries: "
                      << e.what() << std::endl;
            return;
        }
    }

    cleanUVW loc_clean_uvw;

    data_vec = miscFunc_elitpc::groupEntriesByEvent(std::move(data_vec));

    std::cout << "New data size is " << data_vec.size() << "\n\n";

    for (auto &&event : data_vec) {

        event.uvw_data =
            miscFunc_elitpc::mergeSplitStrips(std::move(event.uvw_data));
    }

    int nr_events = data_vec.size();

    std::cout << "There are " << nr_events << " events."
              << "\n\n";

    auto loc_canv =
        new TCanvas("View entries canvas", "View entries", 1500, 1000);
    auto loc_pad = new TPad("pad name", "pad title", 0, 0, 1, 1);
    loc_pad->Divide(3, 1);
    loc_pad->Draw();

    TH2D *u_hists = nullptr;
    TH2D *v_hists = nullptr;
    TH2D *w_hists = nullptr;
    TH1D *c_hist = nullptr;

    int event_nr = -1;

    uint32_t loop_nr = 0;

    while (1) {

        std::cout
            << "Press 'e' to exit, 'd' for next and 'a' for previous and i "
               "to input the entry number. Loop "
            << loop_nr << std::endl;

        loop_nr++;

        char key_val = getchar();

        // Clear the input buffer
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (key_val == 'e') {

            return;

        } else if (key_val == 'd') {

            if (event_nr < (nr_events - 1)) {

                event_nr++;
            }

            std::cout << "Event number is " << event_nr << "\n\n" << std::endl;

        } else if (key_val == 'a') {

            if (event_nr > 0) {
                event_nr--;
            }

            std::cout << "Event number is " << event_nr << "\n\n" << std::endl;

        } else if (key_val == 'i') {

            std::cout << "Please enter the entry number.\n";

            std::cin >> event_nr;

            if (event_nr < 0 || event_nr > (nr_events - 1))
                event_nr = 0;

            std::cout << "Event number is " << event_nr << "\n\n" << std::endl;
        }

        if (event_nr < 0) {
            event_nr = 0;
        }

        if (opt_clean) {

            loc_clean_uvw.setUVWData(data_vec[event_nr].uvw_data);

            auto err =
                loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoU>(true);
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;

            err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoV>(true);
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;

            err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoW>(true);
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;

            data_vec[event_nr].uvw_data = loc_clean_uvw.returnDataUVW();
        }

        // Reset hists

        if (u_hists != nullptr)
            u_hists->Reset();

        if (v_hists != nullptr)
            v_hists->Reset();

        if (w_hists != nullptr)
            w_hists->Reset();

        if (c_hist != nullptr)
            c_hist->Reset();

        // Create new hists

        u_hists = new TH2D(Form("u_hists_%d", event_nr),
                           Form("Histogram for U entry %d", event_nr), 512, 1,
                           513, 250, 1, 251);
        v_hists = new TH2D(Form("v_hists_%d", event_nr),
                           Form("Histogram for V entry %d", event_nr), 512, 1,
                           513, 250, 1, 251);
        w_hists = new TH2D(Form("w_hists_%d", event_nr),
                           Form("Histogram for W entry %d", event_nr), 512, 1,
                           513, 250, 1, 251);

        // Fill the hists

        int bin = 0;

        std::array<double, 512> ch_array = {0};

        std::cout << "\n\nViewing event " << event_nr
                  << " with Id: " << data_vec[event_nr].event_id << " and size "
                  << data_vec[event_nr].uvw_data.size() << "\n\n";

        for (const auto &iter : data_vec[event_nr].uvw_data) {

            // All signals should have size 512;
            if (iter.signal_val.size() == 512) {

                for (auto i = 0; i < 512; ++i) {

                    ch_array[i] += iter.signal_val[i];
                }
            }

            bin = 0;

            if (iter.plane_val == 0 &&
                (iter.strip_section == 0 /*  || iter.strip_section == 1 */)) {

                for (const auto &sig_iter : iter.signal_val) {

                    u_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 1 &&
                (iter.strip_section == 0 /*  || iter.strip_section == 1 */)) {

                for (const auto &sig_iter : iter.signal_val) {

                    v_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 2 &&
                (iter.strip_section == 0 /*  || iter.strip_section == 1 */)) {

                for (const auto &sig_iter : iter.signal_val) {

                    w_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }
        }

        // Draw

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

/// @brief Creates images from the data from the specified root file.
/// @param fileName The root file. Must include path if needed.
/// @param outfolder The loction where the images are created.
/// @param opt_norm Enables the normalization of the channels.
/// @param opt_clean Enables further cleaning and smoothing of the signal.
void drawUVWimages_elitpc(std::vector<TString> fileName, TString outfolder,
                          bool opt_norm = false, bool opt_clean = false)

{

    gROOT->SetBatch(kTRUE);

    std::vector<generalDataStorage> data_vec;

    // ELITPC CoBo files usually come in groups of 4, one for each AsAd, so we
    // have to combine them.
    for (const auto &file_entry : fileName) {
        try {

            // data_vec = miscFunc_elitpc::getAllEntries(file_entry, true,
            // true);
            std::vector<generalDataStorage> temp_data =
                miscFunc_elitpc::getAllEntries(file_entry, true, opt_norm);
            data_vec.insert(data_vec.end(),
                            std::make_move_iterator(temp_data.begin()),
                            std::make_move_iterator(temp_data.end()));
        } catch (const std::exception &e) {

            std::cout << "Encountered error when getting all entries: "
                      << e.what() << std::endl;
            return;
        }
    }

    cleanUVW loc_clean_uvw;

    data_vec = miscFunc_elitpc::groupEntriesByEvent(std::move(data_vec));

    std::cout << "New data size is " << data_vec.size() << "\n\n";

    for (auto &&event : data_vec) {

        event.uvw_data =
            miscFunc_elitpc::mergeSplitStrips(std::move(event.uvw_data));
    }

    int nr_events = data_vec.size();

    std::cout << "There are " << nr_events << " events."
              << "\n\n";

    auto loc_canv = new TCanvas("uvw format", "UVW hists", 800, 600);
    loc_canv->SetFrameLineColor(0);
    loc_canv->SetFrameLineWidth(0);
    loc_canv->SetBottomMargin(0);
    loc_canv->SetTopMargin(0);
    loc_canv->SetLeftMargin(0);
    loc_canv->SetRightMargin(0);

    gStyle->SetOptStat(0);
    // gStyle->SetPalette(kGreyScale);

    TH2D *u_hists = nullptr;
    TH2D *v_hists = nullptr;
    TH2D *w_hists = nullptr;
    TH1D *c_hist = nullptr;

    for (auto event_nr = 0; event_nr < nr_events; ++event_nr) {

        if (opt_clean) {

            loc_clean_uvw.setUVWData(data_vec[event_nr].uvw_data);

            auto err =
                loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoU>(true);
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;

            err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoV>(true);
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;

            err = loc_clean_uvw.substractBl<cleanUVW::miniPlaneInfoW>(true);
            if (err != 0)
                std::cerr << "Error substractBl code " << err << std::endl;

            data_vec[event_nr].uvw_data = loc_clean_uvw.returnDataUVW();
        }

        // Reset hists

        if (u_hists != nullptr)
            u_hists->Reset();

        if (v_hists != nullptr)
            v_hists->Reset();

        if (w_hists != nullptr)
            w_hists->Reset();

        if (c_hist != nullptr)
            c_hist->Reset();

        // Create new hists

        u_hists = new TH2D(Form("u_hists_%d", event_nr), "", 512, 1, 513, 132,
                           1, 133);
        v_hists = new TH2D(Form("v_hists_%d", event_nr), "", 512, 1, 513, 225,
                           1, 226);
        w_hists = new TH2D(Form("w_hists_%d", event_nr), "", 512, 1, 513, 226,
                           1, 227);

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

        u_hists->SetMinimum(-1);
        u_hists->SetMaximum(1000);

        v_hists->SetMinimum(-1);
        v_hists->SetMaximum(1000);

        w_hists->SetMinimum(-1);
        w_hists->SetMaximum(1000);

        // Fill the hists

        int bin = 0;

        std::array<double, 512> ch_array = {0};

        for (const auto &iter : data_vec[event_nr].uvw_data) {

            // All signals should have size 512;
            if (iter.signal_val.size() == 512) {

                for (auto i = 0; i < 512; ++i) {

                    ch_array[i] += iter.signal_val[i];
                }
            }

            bin = 0;

            if (iter.plane_val == 0 &&
                (iter.strip_section == 0 /*  || iter.strip_section == 1 */)) {

                for (const auto &sig_iter : iter.signal_val) {

                    u_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 1 &&
                (iter.strip_section == 0 /*  || iter.strip_section == 1 */)) {

                for (const auto &sig_iter : iter.signal_val) {

                    v_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }

            if (iter.plane_val == 2 &&
                (iter.strip_section == 0 /*  || iter.strip_section == 1 */)) {

                for (const auto &sig_iter : iter.signal_val) {

                    w_hists->SetBinContent(++bin, iter.strip_nr, sig_iter);
                }
            }
        }

        // Draw

        u_hists->Draw("COL");
        loc_canv->SetWindowSize(512, (3 * 132));
        loc_canv->Update();
        loc_canv->Print(outfolder + Form("%d", event_nr) + "_u" + ".png");

        v_hists->Draw("COL");
        loc_canv->SetWindowSize(512, (3 * 225));
        loc_canv->Update();
        loc_canv->Print(outfolder + Form("%d", event_nr) + "_v" + ".png");

        w_hists->Draw("COL");
        loc_canv->SetWindowSize(512, (3 * 226));
        loc_canv->Update();
        loc_canv->Print(outfolder + Form("%d", event_nr) + "_w" + ".png");
    }

    loc_canv->Close();

    gROOT->SetBatch(kFALSE);
}

/// @brief Creates clean images from the specified root files.
/// @param fileList The list of strings of the root files.
/// @param zip_opt Choose if you want to zip the images.
void mass_create_clean_images_elitpc(std::vector<TString> fileList,
                                     bool zip_opt = false, bool opt_norm = true,
                                     bool opt_clean = false) {

    TString dirandfileName = *(fileList.begin());

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

    drawUVWimages_elitpc(fileList, imagePath, opt_norm, opt_clean);

    if (zip_opt) {

        TString zipCommand = ".! zip -j -r -m " + dir + out_dir_name + "/" +
                             fileName + ".zip " + imagePath;

        gROOT->ProcessLine(zipCommand);

        TString rmCommand = ".! rm -r " + imagePath;

        gROOT->ProcessLine(rmCommand);
    }
}

// root -q
// "runmacro_elitpc.cpp({\"/media/gant/Expansion/elitpcraw/CoBo0_AsAd0_2022-04-12T08_03_44.531_0000.root\",
// \"/media/gant/Expansion/elitpcraw/CoBo0_AsAd1_2022-04-12T08_03_44.533_0000.root\",
// \"/media/gant/Expansion/elitpcraw/CoBo0_AsAd2_2022-04-12T08_03_44.536_0000.root\",
// \"/media/gant/Expansion/elitpcraw/CoBo0_AsAd3_2022-04-12T08_03_44.540_0000.root\"})"
// root -q "runmacro_elitpc.cpp({\"a\"})"
void runmacro_elitpc(std::initializer_list<TString> lin_arg) {

    /* viewUVWdata_elitpc(
        {"/media/gant/Cracow_IGN14_Run/20210710_extTrg_CO2_80mbar/"
         "CoBo_ALL_AsAd_ALL_2021-07-10T17:29:59.700_0000.root"}); */

    /* viewUVWdata_elitpc({"/media/gant/Expansion/elitpcraw/"
                        "CoBo0_AsAd0_2022-04-12T08_03_44.531_0000.root",
                        "/media/gant/Expansion/elitpcraw/"
                        "CoBo0_AsAd1_2022-04-12T08_03_44.533_0000.root",
                        "/media/gant/Expansion/elitpcraw/"
                        "CoBo0_AsAd2_2022-04-12T08_03_44.536_0000.root",
                        "/media/gant/Expansion/elitpcraw/"
                        "CoBo0_AsAd3_2022-04-12T08_03_44.540_0000.root"},
                       false, false); */

    /* mass_create_clean_images_elitpc(
        {"/media/gant/Expansion/elitpcraw/"
         "CoBo0_AsAd0_2022-04-12T08_03_44.531_0000.root",
         "/media/gant/Expansion/elitpcraw/"
         "CoBo0_AsAd1_2022-04-12T08_03_44.533_0000.root",
         "/media/gant/Expansion/elitpcraw/"
         "CoBo0_AsAd2_2022-04-12T08_03_44.536_0000.root",
         "/media/gant/Expansion/elitpcraw/"
         "CoBo0_AsAd3_2022-04-12T08_03_44.540_0000.root"},
        false); */

    /* testConvertUVW("/media/gant/Expansion/elitpcraw/"
                   "CoBo_ALL_AsAd_ALL_2021-07-12T11 36 07.452_0000.root"); */

    /* mass_create_raw_images_elitpc(
        "/media/gant/Expansion/elitpcraw/"
        "CoBo_ALL_AsAd_ALL_2021-07-12T11 36 07.452_0000.root"); */

    // mass_create_clean_images_elitpc(lin_arg, true);

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
    std::vector<TString> selected_files(lin_arg_it, lin_arg.end());

    if (function_option == "-view") {

        viewUVWdata_elitpc(selected_files, norm_option, clean_option);
    } else if (function_option == "-convert") {

        mass_create_clean_images_elitpc(selected_files, zip_option, norm_option,
                                        clean_option);
    } else {

        std::cerr << "Error: Invalid view/convert option.";
        return;
    }
}