#ifndef miscFuncelitpc_hpp
#define miscFuncelitpc_hpp 1

#include <vector>

#include "convertUVW_elitpc.hpp"
#include "generalDataStorage.hpp"
#include "loadData.hpp"

/// @brief Contains verious functions used for the processing of data taken from
/// the ELITPC detector.
namespace miscFunc_elitpc {

std::vector<generalDataStorage>
getAllEntries(TString fileName, bool opt_verbose = true, bool opt_norm = false);

std::vector<generalDataStorage>
groupEntriesByEvent(std::vector<generalDataStorage> &&entries_vec);

std::vector<dataUVW> mergeSplitStrips(std::vector<dataUVW> &&data_vec);

};   // namespace miscFunc_elitpc

#endif