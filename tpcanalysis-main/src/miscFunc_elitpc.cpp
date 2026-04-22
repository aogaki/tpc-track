#include "../include/miscFunc_elitpc.hpp"

/// @brief Reads all of the entries from a file and returns a vector with it in
/// the UVW format. The raw data is deleted to save space.
/// @param fileName Name of the '.root' file.
/// @param opt_verbose Prints aditional info in the console.
/// @param opt_norm Set normalization to true or false.
/// @return a generalDataStorage vector containing the UVW events. The raw data
/// is deleted from it to save space.
std::vector<generalDataStorage> miscFunc_elitpc::getAllEntries(TString fileName,
                                                               bool opt_verbose,
                                                               bool opt_norm) {

    int nr_entries = -1;

    auto goodFile = fileName;

    auto goodTree = "tree";

    loadData good_data(goodFile, goodTree);

    auto err = good_data.openFile();

    if (err != 0) {

        std::cerr << "Error at openFile code: " << err << std::endl;

        throw std::runtime_error("Could not open the file.\n");
    }

    err = good_data.readData();

    if (err != 0) {

        std::cerr << "Error at readData code: " << err << std::endl;

        throw std::runtime_error("Could not read the data.\n");
    }

    nr_entries = good_data.returnNEntries();

    if (opt_verbose) {

        std::cout << "Number of entries is " << nr_entries << "\n\n";
    }

    convertUVW_elitpc loc_conv_uvw;

    err = loc_conv_uvw.openSpecFile();

    if (err != 0) {

        std::cerr << "Error at openSpecFile code: " << err << std::endl;

        throw std::runtime_error("Could not open the spec file.\n");
    }

    uint32_t entry_nr = 0;

    std::vector<generalDataStorage> data_vec;

    // Save some time by reserving before.
    data_vec.reserve(nr_entries);

    while (entry_nr < nr_entries) {

        generalDataStorage data_container;

        err = good_data.decodeData(entry_nr, true);

        if (err != 0) {

            std::cout << "Error decode data code " << err << " at entry "
                      << entry_nr << err << std::endl;

            throw std::runtime_error("Could not decode the data.\n");
        }

        data_container.root_raw_data = good_data.returnRawData();

        loc_conv_uvw.setRawData(data_container.root_raw_data);

        err = loc_conv_uvw.makeConversion(opt_norm, false);
        if (err != 0) {

            std::cout << "Make conversion error code " << err << " at entry "
                      << entry_nr << std::endl;

            throw std::runtime_error("Could not make the conversion.\n");
        }

        data_container.uvw_data = loc_conv_uvw.returnDataUVW();

        data_container.event_id = data_container.uvw_data[0].event_id;

        //###TEMPORARY###
        //###############

        for (auto &data_el : data_container.uvw_data) {

            // Remove the first and last few bins.
            std::fill(data_el.signal_val.begin(),
                      data_el.signal_val.begin() + 10, 0);

            std::fill(data_el.signal_val.begin() + 500,
                      data_el.signal_val.begin() + 512, 0);

            // Bring all values that are close to 0 to 10 in order to not have
            // empty bins.
            std::transform(
                data_el.signal_val.begin(), data_el.signal_val.end(),
                data_el.signal_val.begin(),
                [](const double &sig_el) { return std::max(10.0, sig_el); });
        }

        //###############
        //###############

        // Delete useless data to save memory.
        std::vector<rawData>().swap(data_container.root_raw_data);

        data_vec.push_back(data_container);

        if (opt_verbose && (entry_nr % (nr_entries / 10) == 0)) {
            std::cout << "At entry " << entry_nr << "\n";
        }

        entry_nr++;
    }

    if (opt_verbose) {

        std::cout << "\nThe data vector has size: " << data_vec.size()
                  << std::endl;
    }

    return data_vec;
}

/// @brief Some ELITPC data contains separate files from each AsAd boards. This
/// function groups the entries with the same event_id from different files.
/// @param entries_vec The vector containing the data. We only care about the
/// UVW data from it.
/// @return A new vector containing entries from all of the '.root' files
/// combined into one.
std::vector<generalDataStorage> miscFunc_elitpc::groupEntriesByEvent(
    std::vector<generalDataStorage> &&entries_vec) {

    std::multimap<int, std::vector<dataUVW>> event_entry_group;

    for (auto &&entry : entries_vec) {

        event_entry_group.insert({entry.event_id, std::move(entry.uvw_data)});
    }

    // Free as much memory as possible to avoid crashes.
    std::vector<generalDataStorage>().swap(entries_vec);

    std::vector<generalDataStorage>
        result;   // Result vector to store merged data

    generalDataStorage loc_data;

    int prev_id = -1;
    for (auto &event : event_entry_group) {

        if (event.first != prev_id) {

            if (prev_id != -1) {

                result.push_back(std::move(loc_data));
            }
            loc_data.uvw_data.clear();

            loc_data.event_id = event.first;
        }

        loc_data.uvw_data.insert(loc_data.uvw_data.end(),
                                 std::make_move_iterator(event.second.begin()),
                                 std::make_move_iterator(event.second.end()));

        prev_id = event.first;
    }

    return result;
}

/// @brief Some strips in the ELITPC detector are split due to it's size. From
/// what I understood we should handle these split strips as one since we don;t
/// care on which half of the stip the event happens.
/// At the moment we just add the signals for these strips since it seems to
/// work best, but it may be more correct to average the signals instead.
/// @param data_vec The UVW data with split strips.
/// @return The UVW data with the split strips merged.
std::vector<dataUVW>
miscFunc_elitpc::mergeSplitStrips(std::vector<dataUVW> &&data_vec) {

    std::multimap<std::pair<int, int>, size_t> stripSignalMap;

    for (size_t i = 0; i < data_vec.size(); ++i) {
        const auto &data = data_vec[i];
        stripSignalMap.insert({{data.plane_val, data.strip_nr}, i});
    }

    std::vector<dataUVW> averaged_data;

    dataUVW averaged_entry;

    for (auto it = stripSignalMap.begin(); it != stripSignalMap.end();) {

        // Range of entries with the same strip_nr
        auto range = stripSignalMap.equal_range(it->first);

        size_t signal_size = data_vec[it->second].signal_val.size();

        std::vector<double> averaged_signal_val(
            data_vec.front().signal_val.size(), 0.0);

        int count = 0;

        for (auto iter = range.first; iter != range.second; ++iter) {

            size_t index = iter->second;
            const auto &signal_val = data_vec[index].signal_val;

            for (size_t i = 0; i < signal_size; ++i) {
                averaged_signal_val[i] += signal_val[i];
            }
            count++;
        }

        /* for (auto &val : averaged_signal_val) {

            val /= count;
        } */

        averaged_entry.signal_val = std::move(averaged_signal_val);
        averaged_entry.baseline_val = data_vec[it->second].baseline_val;
        averaged_entry.plane_val = data_vec[it->second].plane_val;
        averaged_entry.strip_nr = data_vec[it->second].strip_nr;
        averaged_entry.entry_nr = data_vec[it->second].entry_nr;
        averaged_entry.event_id = data_vec[it->second].event_id;
        averaged_entry.strip_section = 0;

        averaged_data.push_back(std::move(averaged_entry));

        it = range.second;
    }

    return averaged_data;
}
