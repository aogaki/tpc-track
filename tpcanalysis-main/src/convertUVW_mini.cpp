#include "../include/convertUVW_mini.hpp"

/**
 * @brief Construct a new convert U V W::convert U V W object
 *
 * @param data_vec the raw data vector to be converted to the UVW format
 */
/* convertUVW_mini::convertUVW_mini(std::vector<rawData> data_vec) {

    m_data_vec = std::move(data_vec);
} */

/**
 * @brief Sets the raw data vector to a new value.
 *
 * @param data_vec The raw data vector to be set.
 * @return error codes
 */
int convertUVW_mini::setRawData(std::vector<rawData> data_vec) {

    // The size of these vectors should always be the same (256 and 272 if I'm
    // not wrong) so no need to reset the memory.
    // std::vector<rawData>().swap(m_data_vec);
    // std::vector<dataUVW>().swap(m_uvw_vec);
    m_data_vec.clear();
    m_uvw_vec.clear();

    if (data_vec.size() == 0)
        return -3;

    m_data_vec = std::move(data_vec);

    return 0;
}

/**
 * @brief Sets the UVW data vector to a new value.
 * WARNING: This function should only be used for testing purposes if you want
 * to, for some reason, change something about the UVW vector, for example
 * wanting to see the charge histogram for a modified vector.
 *
 * @param data_vec The UVW data vector to be set.
 * @return int error codes
 */
int convertUVW_mini::setUVWData(std::vector<dataUVW> data_vec) {

    // The size of these vectors should always be the same (256 and 272 if I'm
    // not wrong) so no need to reset the memory.
    // std::vector<rawData>().swap(m_data_vec);
    // std::vector<dataUVW>().swap(m_uvw_vec);
    m_data_vec.clear();
    m_uvw_vec.clear();

    if (data_vec.size() == 0)
        return -3;

    m_uvw_vec = std::move(data_vec);

    return 0;
}

/**
 * @brief Opens the specification file and makes the map.
 *
 * @param opt_tpc_ver Selects the tpc version. 0 is the mini TPC and 1 is the
 * large TPC.
 *
 * @return error codes
 */
int convertUVW_mini::openSpecFile() {

    buildNormalizationMap();

    std::ifstream spec_file(m_specfilename.c_str());

    if (!spec_file.is_open()) {
        std::cerr << "Error opening file." << std::endl;
        return -1;
    }

    std::string buf;

    while (std::getline(spec_file, buf)) {

        if (buf[0] == std::string("#")) {
            continue;
        } else if (buf[0] == std::string("U") || buf[0] == std::string("V") ||
                   buf[0] == std::string("W")) {

            std::stringstream line(buf);
            std::string filler;
            std::string plane;
            int strip = INT32_MAX;
            int AGET = INT32_MAX;   // chip nr
            int channel = INT32_MAX;

            line >> plane >> filler >> strip >> filler >> filler >> AGET >>
                channel >> filler >> filler >> filler;

            GEM gem = GEM::size;
            if (plane == std::string("U"))
                gem = GEM::U;
            else if (plane == std::string("V"))
                gem = GEM::V;
            else if (plane == std::string("W"))
                gem = GEM::W;

            auto old_channel = channel;

            try {
                channel = m_channel_reorder_map[channel];
            } catch (...) {
                std::cerr << "Error at " << old_channel << " which links to "
                          << m_channel_reorder_map[old_channel] << std::endl;
            }

            fPositionMap.insert({{AGET, channel}, {gem, strip}});
        }
    }

    return 0;
}

/**
 * @brief Makes the conversion to the UVW format.
 *
 * @param opt_norm Choose to normalize the channels or not.
 * @param opt_verbose Toggle the verbose option which shows the channels that
 * are not in the map. It should normally not be needes since the only channels
 * not in the map should be the FPN channels. However, you can toggle it if you
 * want to see that no other channels are left out.
 * @return error codes
 */
int convertUVW_mini::makeConversion(const bool opt_norm,
                                    const bool opt_verbose) {

    auto err_code = 0;

    dataUVW loc_data_uvw;

    if (m_data_vec.size() == 0)
        return -3;

    for (const auto &data_inst : m_data_vec) {

        try {
            auto uvwPosition =
                fPositionMap.at({data_inst.chip_nr, data_inst.ch_nr});
            loc_data_uvw.plane_val = int(uvwPosition.first);
            loc_data_uvw.strip_section = 0;
            loc_data_uvw.strip_nr = int(uvwPosition.second);
            loc_data_uvw.signal_val = data_inst.signal_val;
            loc_data_uvw.entry_nr = data_inst.entry_nr;
            loc_data_uvw.event_id = data_inst.event_id;
            m_uvw_vec.push_back(loc_data_uvw);

        } catch (...) {

            // These channels are probably just the FPN channels, but it's good
            // to check.
            if (opt_verbose) {
                std::cout << "AGET: " << data_inst.chip_nr
                          << "\tChannel: " << data_inst.ch_nr
                          << " is not assigned." << std::endl;
            }
        }
    }

    if (opt_norm) {
        normalizeChannels();
    }

    return err_code;
}

/**
 * @brief Returns the dataUVW vector containing the converted data.
 *
 * @return std::vector<dataUVW> the vector to be returned containing the data
 * converted to UVW
 */
std::vector<dataUVW> convertUVW_mini::returnDataUVW() const {
    return m_uvw_vec;
}

/**
 * @brief Writes the data from m_uvw_vec to a .csv file.
 *  The format of the .csv file is the following:
 *  First column is the plane value.
 *  Second colums is the strip number.
 *  Third column in the entry number. (Sould be the same for every element)
 *  The remaining columns contain the values of the signal.
 *
 * @param file_name the name of the .csv file
 * @return int error codes
 */
int convertUVW_mini::convertToCSV(const std::string file_name) {

    std::ofstream out_file(file_name);

    if (!out_file.is_open()) {
        std::cerr << "Error: Could not open file for output" << std::endl;
        return -1;
    }

    // header
    out_file << "plane_val,strip_nr,entry_nr,signal_val\n";

    for (const auto &data_entry : m_uvw_vec) {

        out_file << data_entry.plane_val << "," << data_entry.strip_nr << ","
                 << data_entry.entry_nr << ",";

        for (auto i = 0; static_cast<std::vector<double>::size_type>(i) <
                         data_entry.signal_val.size();
             i++) {

            out_file << data_entry.signal_val[i];
            if (static_cast<std::vector<double>::size_type>(i) <
                data_entry.signal_val.size() - 1) {
                out_file << ",";
            }
        }

        out_file << "\n";
    }

    out_file.close();

    return 0;
}

/**
 * @brief Uses the ch_norm_ratios to build a std::map used when normalizing the
 * channels.
 *
 */
void convertUVW_mini::buildNormalizationMap() {

    // const std::string filename = "./utils/ch_norm_ratios.csv";

    const std::string filename = m_norm_file_name;

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Failed to open file " << filename << std::endl;
        return;
    }

    // Read and discard the first line
    std::string line;
    std::getline(file, line);

    // Parse the data from the file and add it to the map
    while (std::getline(file, line)) {

        std::istringstream iss(line);
        std::string field;
        int plane, strip;
        double ratio;

        std::getline(iss, field, ',');
        plane = std::stoi(field);
        std::getline(iss, field, ',');
        strip = std::stoi(field);
        std::getline(iss, field, ',');
        ratio = std::stod(field);

        // m_ch_ratio_map[std::make_pair(plane, strip)] = ratio;
        m_ch_ratio_map.insert({std::make_pair(plane, strip), ratio});
    }

    file.close();
}

/**
 * @brief Uses the ch_norm_ratios.csv file to normalize the Channels. Each
 * signal on each channel is divided by a channel specific ratio. This is done
 * in order to make sure the values on each channel are proportional with each
 * other.
 */
void convertUVW_mini::normalizeChannels() {

    if (m_ch_ratio_map.size() == 0) {

        std::cerr << "Error map is empty." << std::endl;
        return;
    }

    if (m_uvw_vec.size() == 0) {

        std::cerr << "Error UVW vector is empty." << std::endl;
        return;
    }

    for (auto &uvw_entry : m_uvw_vec) {

        if (uvw_entry.plane_val == 0) {

            try {
                auto loc_ratio = m_ch_ratio_map.at({0, uvw_entry.strip_nr});

                std::transform(
                    uvw_entry.signal_val.begin(), uvw_entry.signal_val.end(),
                    uvw_entry.signal_val.begin(),
                    [loc_ratio](double sig_el) { return sig_el / loc_ratio; });
            } catch (...) {
                std::cerr
                    << "Error: Out of bounds at normalizeChannels() -> U, "
                    << uvw_entry.strip_nr << std::endl;
                return;
            }

        } else if (uvw_entry.plane_val == 1) {

            try {
                auto loc_ratio = m_ch_ratio_map.at({1, uvw_entry.strip_nr});

                std::transform(
                    uvw_entry.signal_val.begin(), uvw_entry.signal_val.end(),
                    uvw_entry.signal_val.begin(),
                    [loc_ratio](double sig_el) { return sig_el / loc_ratio; });
            } catch (...) {
                std::cerr
                    << "Error: Out of bounds at normalizeChannels() -> U, "
                    << uvw_entry.strip_nr << std::endl;
                return;
            }

        } else if (uvw_entry.plane_val == 2) {

            try {
                auto loc_ratio = m_ch_ratio_map.at({2, uvw_entry.strip_nr});

                std::transform(
                    uvw_entry.signal_val.begin(), uvw_entry.signal_val.end(),
                    uvw_entry.signal_val.begin(),
                    [loc_ratio](double sig_el) { return sig_el / loc_ratio; });
            } catch (...) {
                std::cerr
                    << "Error: Out of bounds at normalizeChannels() -> U, "
                    << uvw_entry.strip_nr << std::endl;
                return;
            }
        }
    }
}
