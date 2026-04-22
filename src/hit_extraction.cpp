#include "tpctrack/hit_extraction.hpp"

namespace tpctrack {

std::vector<Hit> extractHits(const std::vector<StripSignal>& signals,
                             double threshold) {
    std::vector<Hit> hits;
    for (const auto& s : signals) {
        const int n = static_cast<int>(s.signal.size());
        for (int t = 0; t < n; ++t) {
            if (s.signal[t] > threshold) {
                hits.push_back({s.plane, s.strip, t, s.signal[t]});
            }
        }
    }
    return hits;
}

}  // namespace tpctrack
