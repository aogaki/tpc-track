#pragma once

#include <vector>

namespace tpctrack {

struct Hit {
    int plane;
    int strip;
    int time_bin;
    double charge;
};

struct StripSignal {
    int plane;
    int strip;
    std::vector<double> signal;
};

}  // namespace tpctrack
