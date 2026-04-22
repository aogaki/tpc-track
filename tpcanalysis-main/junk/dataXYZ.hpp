#ifndef dataXYZ_hpp
#define dataXYZ_hpp 1

#include <numeric>

/**
 * @brief Used for data storage.
 * The information is stored here in the XYZ format.
 *
 */
class dataXYZ {

  public:
    dataXYZ(){};
    ~dataXYZ(){};

    /// @brief The location on X.
    double data_x = std::numeric_limits<double>::max();

    /// @brief The location on Y.
    double data_y = std::numeric_limits<double>::max();

    /// @brief The location on Z.
    double data_z = std::numeric_limits<double>::max();

    /// @brief The value of the charge.
    double data_charge = std::numeric_limits<double>::max();
};

#endif