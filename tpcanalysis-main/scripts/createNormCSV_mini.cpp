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

/**
 * @brief Creates the .csv file used to normalize the channels.
 * The file and channels used have been pre selected.
 * If you need to generate a new ratio csv file then follow the steps:
 * - Choose some entries that contain only noise.
 * - Edit the sample_size variable with the correct number of samples.
 * - Edit entry_nrs with the correct entries. Make sure the number is exactly
 * the number specified by sample size.
 *
 */
void createNormCSV_mini(bool lin_arg = true) {

    const bool opt_fpn = lin_arg;

    TString filename = "/media/gant/Expansion/tpc_root_raw/DATA_ROOT/"
                       "CoBo_2018-06-20T10-35-30.853_0005.root";

    auto goodTree = "tree";

    loadData good_data(filename, goodTree);

    auto err = good_data.openFile();

    err = good_data.readData();

    auto max_entries = good_data.returnNEntries();

    convertUVW_mini loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0)
        return;

    generalDataStorage data_container;

    const int sample_size = 20;

    // we always use 20 entries
    std::array<std::array<double, sample_size>, 3> entry_mean = {{}};

    std::array<double, 3> total_mean = {};

    // Entries have been manually selected and should be mostly noise.
    std::array<int, sample_size> entry_nrs = {7,  15,  22,  36,  37,  40, 49,
                                              52, 53,  55,  56,  61,  65, 68,
                                              78, 102, 103, 143, 152, 161};

    for (auto i = 0; i < sample_size; i++) {

        auto err = good_data.decodeData(entry_nrs[i], opt_fpn);
        if (err != 0) {
            std::cout << "Error decode data code " << err << std::endl;
        }

        data_container.root_raw_data = good_data.returnRawData();

        std::cout << "RAW data size is " << data_container.root_raw_data.size()
                  << std::endl;

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion(false);
        if (err != 0)
            std::cout << "Make conversion error code " << err << std::endl;

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        int count_u = 0;
        int count_v = 0;
        int count_w = 0;

        for (const auto &uvw_entry : data_container.uvw_data) {

            if (uvw_entry.plane_val == 0) {

                entry_mean[0][i] +=
                    std::accumulate(uvw_entry.signal_val.begin(),
                                    uvw_entry.signal_val.end(), 0.0) /
                    uvw_entry.signal_val.size();

                count_u++;

            } else if (uvw_entry.plane_val == 1) {

                entry_mean[1][i] +=
                    std::accumulate(uvw_entry.signal_val.begin(),
                                    uvw_entry.signal_val.end(), 0.0) /
                    uvw_entry.signal_val.size();

                count_v++;

            } else if (uvw_entry.plane_val == 2) {

                entry_mean[2][i] +=
                    std::accumulate(uvw_entry.signal_val.begin(),
                                    uvw_entry.signal_val.end(), 0.0) /
                    uvw_entry.signal_val.size();

                count_w++;
            }
        }

        entry_mean[0][i] /= count_u;
        entry_mean[1][i] /= count_v;
        entry_mean[2][i] /= count_w;
    }

    total_mean[0] =
        std::accumulate(entry_mean[0].begin(), entry_mean[0].end(), 0.0) /
        entry_mean[0].size();
    total_mean[1] =
        std::accumulate(entry_mean[1].begin(), entry_mean[1].end(), 0.0) /
        entry_mean[1].size();
    total_mean[2] =
        std::accumulate(entry_mean[2].begin(), entry_mean[2].end(), 0.0) /
        entry_mean[2].size();

    std::array<std::array<std::array<double, sample_size>, 93>, 3>
        entry_ch_ratio = {{{}}};

    std::array<std::array<double, 93>, 3> total_ch_ratio = {{}};

    for (auto i = 0; i < sample_size; i++) {

        auto err = good_data.decodeData(entry_nrs.at(i), opt_fpn);

        data_container.root_raw_data = good_data.returnRawData();

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion(false);

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        for (const auto &uvw_entry : data_container.uvw_data) {

            if (uvw_entry.plane_val == 0) {

                double loc_mean =
                    std::accumulate(uvw_entry.signal_val.begin(),
                                    uvw_entry.signal_val.end(), 0.0) /
                    uvw_entry.signal_val.size();

                entry_ch_ratio[0][uvw_entry.strip_nr][i] =
                    loc_mean / total_mean[0];

            } else if (uvw_entry.plane_val == 1) {

                double loc_mean =
                    std::accumulate(uvw_entry.signal_val.begin(),
                                    uvw_entry.signal_val.end(), 0.0) /
                    uvw_entry.signal_val.size();

                entry_ch_ratio[1][uvw_entry.strip_nr][i] =
                    loc_mean / total_mean[1];

            } else if (uvw_entry.plane_val == 2) {

                double loc_mean =
                    std::accumulate(uvw_entry.signal_val.begin(),
                                    uvw_entry.signal_val.end(), 0.0) /
                    uvw_entry.signal_val.size();

                entry_ch_ratio[2][uvw_entry.strip_nr][i] =
                    loc_mean / total_mean[2];
            }
        }
    }

    std::ofstream out_file("./utils/ch_norm_ratios_mini.csv");

    out_file << "plane,strip,ratio\n";

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 93; j++) {
            double sum = std::accumulate(entry_ch_ratio[i][j].begin(),
                                         entry_ch_ratio[i][j].end(), 0.0);
            total_ch_ratio[i][j] = sum / entry_ch_ratio[i][j].size();

            out_file << i << "," << j << "," << total_ch_ratio[i][j] << "\n";
        }
    }

    out_file.close();
}