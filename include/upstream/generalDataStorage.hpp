#ifndef generalDataStorage_hpp
#define generalDataStorage_hpp 1

#include <numeric>
#include <vector>

#include "dataUVW.hpp"
/* #include "dataXYZ.hpp"
#include "hitPoints.hpp" */
#include "rawData.hpp"

#include "TH1.h"

/**
 * @brief A general data storage class that contains object of different types.
 * Each object of generalDataStorage type must only contain 1 entry from the
 * tree.
 *
 */
class generalDataStorage {
  public:
    /// @brief Stores the raw data from the root file.
    std::vector<rawData> root_raw_data;

    /// @brief Stores the data in the UVW format.
    std::vector<dataUVW> uvw_data;

    /*     /// @brief Stores the data in the XYZ format.
        std::vector<dataXYZ> xyz_data;

        /// @brief Stores the hit information.
        std::vector<hitPoints> hit_data;

        /// @brief Stores the histograms for each channel in an entry. Redundant
       and
        /// will be removed.
        std::vector<TH1D *> raw_hist_container;

        /// @brief The label obtained from filterEventsXY. At the moment it can
       be 1
        /// or 2.
        int filter_label = 0;

        /// @brief Obtained from convertHitData::containsVerticalLine(). Should
       be 1
        /// or 0.
        int contains_vertical_line = -1;

        /// @brief The calculated value of th Mean Square Error
        double mse_value = std::numeric_limits<double>::max(); */

    /// @brief  The number of the entry from the tree. Used for mini-eTPC.
    // uint32_t entry_nr = 0;

    /// @brief The id of the event to which this entry belongs to.
    uint32_t event_id = 0;
};

#endif