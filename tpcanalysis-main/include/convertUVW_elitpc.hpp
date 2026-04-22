#ifndef convertUVWelitpc_hpp
#define convertUVWelitpc_hpp 1

#include <fstream>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>

#include "dataUVW.hpp"
#include "generalDataStorage.hpp"
#include "loadData.hpp"
#include "rawData.hpp"

#include "TCanvas.h"
#include "TH1.h"
#include "TSpectrum.h"

/**
 * @brief Uses the '' file to find the strip and plane
 * from the chip and channel, thus bringing it to the UVW format.
 * It can also normalize the channels.
 */
class convertUVW_elitpc {

  public:
    /* convertUVW_elitpc(){};
    convertUVW_elitpc(std::vector<rawData> data_vec);
    ~convertUVW_elitpc(){}; */

    int setRawData(std::vector<rawData> data_vec);

    int setUVWData(std::vector<dataUVW> data_vec);

    int openSpecFile();

    int makeConversion(const bool opt_norm = true,
                       const bool opt_verbose = false);

    std::vector<dataUVW> returnDataUVW() const;

  private:
    /// @brief The map used to make the conversion. Built from
    /// 'geometry_ELITPC_80mbar_50MHz.dat'.
    std::map<std::tuple<int, int, int>, std::tuple<int, int, int>> fPositionMap;

    /// @brief The vector containing the raw data. Taken as input
    std::vector<rawData> m_data_vec;

    /// @brief The vector containing the converted data. Is the output.
    std::vector<dataUVW> m_uvw_vec;

    /// @brief The charge histogram. Not used currently but can be used in the
    /// future.
    TH1D *m_charge_hist = nullptr;

    /// @brief Used for saving the data found in the normalization csv.
    std::map<std::pair<int, int>, double> m_ch_ratio_map;

    /// @brief The channels in the new_geometry files are from 0 to 63, while
    /// the channels in the files are from 0 to 67. This is because the
    /// new_geometry file doesn't include channels 11, 22, 45, 56 which are used
    /// for Fixed pattern noise. For this reason we need to change the channels
    /// from new_geometry to fit the channels from the files.
    inline static std::map<int, int> m_channel_reorder_map = {
        {0, 0},   {1, 1},   {2, 2},   {3, 3},   {4, 4},   {5, 5},   {6, 6},
        {7, 7},   {8, 8},   {9, 9},   {10, 10}, {11, 12}, {12, 13}, {13, 14},
        {14, 15}, {15, 16}, {16, 17}, {17, 18}, {18, 19}, {19, 20}, {20, 21},
        {21, 23}, {22, 24}, {23, 25}, {24, 26}, {25, 27}, {26, 28}, {27, 29},
        {28, 30}, {29, 31}, {30, 32}, {31, 33}, {32, 34}, {33, 35}, {34, 36},
        {35, 37}, {36, 38}, {37, 39}, {38, 40}, {39, 41}, {40, 42}, {41, 43},
        {42, 44}, {43, 46}, {44, 47}, {45, 48}, {46, 49}, {47, 50}, {48, 51},
        {49, 52}, {50, 53}, {51, 54}, {52, 55}, {53, 57}, {54, 58}, {55, 59},
        {56, 60}, {57, 61}, {58, 62}, {59, 63}, {60, 64}, {61, 65}, {62, 66},
        {63, 67}};

    /// @brief The geometry file used to identify the plane and channel.
    inline static const std::string m_specfilename =
        "./utils/geometry_ELITPC.dat";

    /// @brief The file containing the ratios of each channel for normalization
    /// purposes.
    inline static const std::string m_norm_file_name =
        "./utils/ch_norm_ratios_elitpc.csv";

    void normalizeChannels();

    void buildNormalizationMap();
};

#endif