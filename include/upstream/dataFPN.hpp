#ifndef dataFPN_hpp
#define dataFPN_hpp 1

#include <cstdint>
#include <vector>

/**
 * @brief This class stores the signals from the Fixed Pattern Noise channels.
 *
 */
class dataFPN {
  public:
    /// @brief The number of the entry the signal belongs to.
    int entry_nr = INT32_MAX;

    /// @brief The signal vector.
    std::vector<double> signal_val;
};

#endif