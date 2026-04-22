#pragma once

#include <vector>

#include "tpctrack/hit.hpp"

namespace tpctrack {

std::vector<Hit> extractHits(const std::vector<StripSignal>& signals,
                             double threshold);

}  // namespace tpctrack
