#ifndef rawData_hpp
#define rawData_hpp 1

#include <cstdint>
#include <vector>

/**
 * @brief Used for data storage.
 * Contains the raw data extracted from the .root file.
 *
 */
class rawData {
  public:
    /// @brief The channel number. Must use new_geometry_mini_eTPC.dat to make
    /// sense of it.
    int ch_nr = 0;

    /// @brief The chip number. Must use new_geometry_mini_eTPC.dat to make
    /// sense of it.
    int chip_nr = 0;

    /// @brief The enry from the TTree from which the signal originates.
    int entry_nr = 0;

    /// @brief The id of the AsAd board. Always the same for miniTPC.
    int asad_id = 0;

    /// @brief The event from which the entry comes.
    int event_id = 0;

    /// @brief The signal.
    std::vector<double> signal_val;
};

#endif