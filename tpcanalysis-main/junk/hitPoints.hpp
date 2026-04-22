#ifndef hitPoints_hpp
#define hitPoints_hpp 1

#include "TROOT.h"
#include <vector>

/**
 * @brief Used for data storage.
 * Stores information about the detected hits.
 *
 */
class hitPoints {
  public:
    hitPoints(){};
    ~hitPoints(){};

    /// @brief The strip on which the hit was detected.
    int strip = -1;

    /// @brief The plane on which the hit was detected.
    int plane = -1;

    /// @brief The baseline extracted in convertUVW::substractBl. May change in
    /// the future
    int base_line = 0;

    /// @brief The entry from the .root file from which the signal originates.
    int entry_nr = -1;

    /// @brief The location on the X axis where the hit was detected. The value
    /// represents bin number.
    Double_t peak_x = -1;

    /// @brief The height of the peak.
    Double_t peak_y = -1;

    /// @brief The full width at half maximum of the hit.
    Double_t fwhm = -1;
};

#endif