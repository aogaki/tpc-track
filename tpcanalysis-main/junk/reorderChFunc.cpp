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
 * DEPRECATED:
 * @brief Tools used to reorder the channels to the correct order.
 * No longer useful.
 *
 */
namespace reorderCh {
/**
 * @brief Contains constants used for the U plane.
 *
 */
struct planeInfo_U {

    static const int plane_nr = 0;
    static const int plane_size = 72;
    static const int cfg_size = 3;
    static const int right_angle = 0;
    inline static const std::string plane_file = "./utils/channelorder_u.txt";
    inline static const std::string cfg_file = "./utils/cfg_entries_u.txt";
};

/**
 * @brief Contains constants used for the V plane.
 *
 */
struct planeInfo_V {

    static const int plane_nr = 1;
    static const int plane_size = 92;
    static const int cfg_size = 1;
    static const int right_angle = 0;
    inline static const std::string plane_file = "./utils/channelorder_v.txt";
    inline static const std::string cfg_file = "./utils/cfg_entries_v.txt";
};

/**
 * @brief Contains constants used for the W plane.
 *
 */
struct planeInfo_W {

    static const int plane_nr = 2;
    static const int plane_size = 92;
    static const int cfg_size = 1;
    static const int right_angle = 1;
    inline static const std::string plane_file = "./utils/channelorder_w.txt";
    inline static const std::string cfg_file = "./utils/cfg_entries_w.txt";
};

/**
 * @brief Uses the channelorder_*.txt files from ./utils/ to reorder the
 * channels to the correct order. Works for 1 plane at a time.
 *
 * @tparam pI One of the planeInfo_U, planeInfo_V or planeInfo_W structs,
 * depending on the plane to be reordered.
 * @param uvw_vec The vector to be reordered.
 */
template <typename pI> void reorderChfromFile(std::vector<dataUVW> &uvw_vec) {

    std::ifstream file(pI::plane_file.c_str());

    if (!file.is_open()) {
        std::cerr << "Error: Failed to open file " << pI::plane_file
                  << std::endl;
        return;
    }

    int index = 1;

    std::string line;

    std::map<int, int> ch_map;

    while (std::getline(file, line)) {

        if (ch_map.count(std::stoi(line)) == 0) {
            ch_map.insert({std::stoi(line), index});
        } else {
            std::cerr
                << "Error: key already exists in map at reorderChfromFile."
                << std::endl;
        }

        ++index;
    }

    for (auto &uvw_data : uvw_vec) {

        if (uvw_data.plane_val == pI::plane_nr)
            uvw_data.strip_nr = ch_map.at(uvw_data.strip_nr);
    }

    file.close();
}

/**
 * @brief Calculates the order in which the channels need to be reordered. Only
 * use this if you want to find a better order. The file used is taken as an
 * argument and the entries used are taken from ./utils/cfg_entries_*.txt. The
 * number of entries need to be changed in the planeInfo_* structs.
 *
 * @tparam pI One of the planeInfo_U, planeInfo_V or planeInfo_W structs,
 * depending on the plane to be reordered.
 * @param fileName The .root file used for the reorder.
 */
template <typename pI> void calculateReorder(TString fileName) {

    auto goodFile = fileName;

    auto goodTree = "tree";

    generalDataStorage data_container;

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    auto max_entries = good_data.returnNEntries();

    convertUVW loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    cleanUVW loc_clean_uvw;

    std::map<int, std::array<int, pI::cfg_size>> strip_order_map;

    std::ifstream file(pI::cfg_file.c_str());

    if (!file.is_open()) {
        std::cerr << "Error: Failed to open file " << pI::cfg_file << std::endl;
        return;
    }

    int index = 0;

    std::string line;

    while (std::getline(file, line)) {

        int entry_nr = std::stoi(line);

        err = good_data.decodeData(entry_nr);
        if (err != 0) {
            std::cout << "Error decode data code " << err << std::endl;
        }

        data_container.root_raw_data = good_data.returnRawData();

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion();

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        /* std::cout << "UVW data size is " << data_container.uvw_data.size()
                  << std::endl; */

        err = loc_clean_uvw.setUVWData(data_container.uvw_data);
        if (err != 0)
            std::cerr << "Error clean::setUVWData code " << err << std::endl;

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

        // Order by plane and strip
        std::sort(data_container.uvw_data.begin(),
                  data_container.uvw_data.end(),
                  [](const dataUVW &a, const dataUVW &b) {
                      return 1e6 * a.plane_val + a.strip_nr <
                             1e6 * b.plane_val + b.strip_nr;
                  });

        std::array<std::array<std::size_t, 93>, 3> max_index;

        for (const auto &uvw_el : data_container.uvw_data) {

            auto max_it = std::max_element(uvw_el.signal_val.begin(),
                                           uvw_el.signal_val.end());
            max_index[uvw_el.plane_val][uvw_el.strip_nr] =
                std::distance(uvw_el.signal_val.begin(), max_it);
        }

        if (pI::right_angle) {
            std::sort(data_container.uvw_data.begin(),
                      data_container.uvw_data.end(),
                      [max_index](const dataUVW &a, const dataUVW &b) {
                          return max_index[a.plane_val][a.strip_nr] <
                                 max_index[b.plane_val][b.strip_nr];
                      });
        } else {
            std::sort(data_container.uvw_data.begin(),
                      data_container.uvw_data.end(),
                      [max_index](const dataUVW &a, const dataUVW &b) {
                          return max_index[a.plane_val][a.strip_nr] >
                                 max_index[b.plane_val][b.strip_nr];
                      });
        }

        int val_counter = 0;

        for (auto i = 0; i < data_container.uvw_data.size(); i++) {

            if (data_container.uvw_data.at(i).plane_val == pI::plane_nr) {

                ++val_counter;

                strip_order_map[data_container.uvw_data.at(i).strip_nr][index] =
                    val_counter;
            }
        }

        ++index;
    }

    file.close();

    std::map<int, int> strip_order;

    for (const auto &map_entry : strip_order_map) {

        auto &strip_values = map_entry.second;

        // Create a map to count the frequency of each value.
        std::unordered_map<int, int> frequency_map;
        for (int value : strip_values) {
            frequency_map[value]++;
        }

        std::vector<std::pair<int, int>> frequency_pairs(frequency_map.begin(),
                                                         frequency_map.end());
        std::sort(
            frequency_pairs.begin(), frequency_pairs.end(),
            [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
                return a.second > b.second;
            });

        int most_frequent_value = 0;
        for (const auto &pair : frequency_pairs) {
            if (strip_order.count(pair.first) == 0) {
                most_frequent_value = pair.first;
                break;
            }
        }

        if (strip_order.count(most_frequent_value) == 0) {
            strip_order.insert({most_frequent_value, map_entry.first});
        } else {
            std::cerr << "Error: key " << most_frequent_value
                      << " already exists in map at calculateReorder."
                      << std::endl;
        }
    }

    std::ofstream out_file(pI::plane_file);

    for (const auto &map_entry : strip_order) {

        out_file << map_entry.second << "\n";
    }

    out_file.close();
}

}   // namespace reorderCh

/**
 * @brief For testing purposes only. Helps view the reordered channels.
 *
 * @param fileName The name of the .root file.
 * @param entry_nr The entry number to be viewed.
 * @param plane The plane to be viewed.
 * @param norm_opt The normalization option. 1 if you want the channels to be
 * normalized, else 0.s
 */
void printReorderedChannels(TString fileName, int entry_nr, int plane) {

    auto goodFile = fileName;

    auto goodTree = "tree";

    generalDataStorage data_container;

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();
    err = good_data.readData();
    auto max_entries = good_data.returnNEntries();

    convertUVW loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

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

    generalDataStorage data_container_raw;

    data_container_raw.uvw_data = loc_conv_uvw.returnDataUVW();

    data_container.uvw_data = loc_conv_uvw.returnDataUVW();

    std::cout << "UVW data size is " << data_container.uvw_data.size()
              << std::endl;

    auto loc_canv =
        new TCanvas("View entries canvas", "View entries", 1500, 1000);
    auto loc_pad = new TPad("pad name", "pad title", 0, 0, 1, 1);
    loc_pad->Divide(3, 3);
    loc_pad->Draw();

    TH2D *u_hists = nullptr;
    TH2D *v_hists = nullptr;
    TH2D *w_hists = nullptr;

    TH2D *u_hists_ord = nullptr;
    TH2D *v_hists_ord = nullptr;
    TH2D *w_hists_ord = nullptr;

    TH2D *u_hists_raw = nullptr;
    TH2D *v_hists_raw = nullptr;
    TH2D *w_hists_raw = nullptr;

    u_hists = new TH2D(Form("u_hists_%d", entry_nr),
                       Form("Histogram for U entry %d", entry_nr), 512, 1, 513,
                       100, 1, 101);
    v_hists = new TH2D(Form("v_hists_%d", entry_nr),
                       Form("Histogram for V entry %d", entry_nr), 512, 1, 513,
                       100, 1, 101);
    w_hists = new TH2D(Form("w_hists_%d", entry_nr),
                       Form("Histogram for W entry %d", entry_nr), 512, 1, 513,
                       100, 1, 101);

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

    loc_pad->cd(4);
    u_hists->Draw("COLZ");
    loc_pad->cd(5);
    v_hists->Draw("COLZ");
    loc_pad->cd(6);
    w_hists->Draw("COLZ");

    generalDataStorage data_container_ord = data_container;

    reorderCh::reorderChfromFile<reorderCh::planeInfo_U>(
        data_container_ord.uvw_data);
    reorderCh::reorderChfromFile<reorderCh::planeInfo_V>(
        data_container_ord.uvw_data);
    reorderCh::reorderChfromFile<reorderCh::planeInfo_W>(
        data_container_ord.uvw_data);

    u_hists_ord = new TH2D(Form("u_ord_hists_%d", entry_nr),
                           Form("Histogram ord for U entry %d", entry_nr), 512,
                           1, 513, 100, 1, 101);
    v_hists_ord = new TH2D(Form("v_ord_hists_%d", entry_nr),
                           Form("Histogram ord for V entry %d", entry_nr), 512,
                           1, 513, 100, 1, 101);
    w_hists_ord = new TH2D(Form("w_ord_hists_%d", entry_nr),
                           Form("Histogram ord for W entry %d", entry_nr), 512,
                           1, 513, 100, 1, 101);

    for (const auto &iter : data_container_ord.uvw_data) {

        bin = 0;

        if (iter.plane_val == 0) {

            for (const auto &sig_iter : iter.signal_val) {

                u_hists_ord->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 1) {

            for (const auto &sig_iter : iter.signal_val) {

                v_hists_ord->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 2) {

            for (const auto &sig_iter : iter.signal_val) {

                w_hists_ord->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }
    }

    loc_pad->cd(7);
    u_hists_ord->Draw("COLZ");
    loc_pad->cd(8);
    v_hists_ord->Draw("COLZ");
    loc_pad->cd(9);
    w_hists_ord->Draw("COLZ");

    loc_conv_uvw.setUVWData(data_container_ord.uvw_data);

    u_hists_raw = new TH2D(Form("u_raw_hists_%d", entry_nr),
                           Form("Histogram raw for U entry %d", entry_nr), 512,
                           1, 513, 100, 1, 101);
    v_hists_raw = new TH2D(Form("v_raw_hists_%d", entry_nr),
                           Form("Histogram raw for V entry %d", entry_nr), 512,
                           1, 513, 100, 1, 101);
    w_hists_raw = new TH2D(Form("w_raw_hists_%d", entry_nr),
                           Form("Histogram raw for W entry %d", entry_nr), 512,
                           1, 513, 100, 1, 101);

    for (const auto &iter : data_container_raw.uvw_data) {

        bin = 0;

        if (iter.plane_val == 0) {

            for (const auto &sig_iter : iter.signal_val) {

                u_hists_raw->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 1) {

            for (const auto &sig_iter : iter.signal_val) {

                v_hists_raw->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }

        if (iter.plane_val == 2) {

            for (const auto &sig_iter : iter.signal_val) {

                w_hists_raw->SetBinContent(++bin, iter.strip_nr, sig_iter);
            }
        }
    }

    loc_pad->cd(1);
    u_hists_raw->Draw("COLZ");
    loc_pad->cd(2);
    v_hists_raw->Draw("COLZ");
    loc_pad->cd(3);
    w_hists_raw->Draw("COLZ");

    loc_canv->Update();

    loc_canv->WaitPrimitive();
}

void reorderChFun(
    TString
        lin_arg) { /* reorderCh::calculateReorder<reorderCh::planeInfo_U>(
             "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
             "CoBo_2018-06-20T10-35-30.853_0005.root");
             reorderCh::calculateReorder<reorderCh::planeInfo_V>(
             "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
             "CoBo_2018-06-20T10-35-30.853_0005.root");
             reorderCh::calculateReorder<reorderCh::planeInfo_W>(
             "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
             "CoBo_2018-06-20T10-35-30.853_0005.root");

             printReorderedChannels("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                          "CoBo_2018-06-20T10-35-30.853_0005.root",
                          174, 0, 1); */

    printReorderedChannels("/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                           "CoBo_2018-06-20T10-51-39.459_0000.root",
                           79, 0);
}