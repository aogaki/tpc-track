#include "../include/cleanUVW.hpp"

/**
 * @brief Construct a new clean U V W::clean U V W object.
 * Sets the vector containing the uvw data to a new value.
 * Initializes the size of m_coefficients to the window size.
 *
 * @param uvw_vec The vector containing the UVW data to be cleaned.
 */
/* cleanUVW::cleanUVW(std::vector<dataUVW> uvw_vec)
    : m_coefficients(m_window_size) {

    // Compute the Savitzky-Golay coefficients for smoothing.
    // Only needs to be done once.
    computeSavitzkyGolayCoefficients();

    // The size of uvw should always be the same so we can just clear.
    // std::vector<dataUVW>().swap(m_uvw_vec);
    m_uvw_vec.clear();

    m_uvw_vec = std::move(uvw_vec);
} */

/**
 * @brief Sets the UVW data vector to a new vector.
 *
 * @param uvw_vec The new vector.
 * @return int Error codes.
 */
int cleanUVW::setUVWData(std::vector<dataUVW> uvw_vec) {

    // The size of uvw should always be the same so we can just clear.
    // std::vector<dataUVW>().swap(m_uvw_vec);
    m_uvw_vec.clear();

    if (uvw_vec.size() == 0)
        return -3;

    m_uvw_vec = std::move(uvw_vec);

    return 0;
}

/**
 * @brief Substracts the baseline and smooths the signal using the
 * Savitzky-Golay filter twice. This has been the best way I could find of
 * cleaning the signal.
 *
 * @tparam pI This parameter should be one of the three structs defined in the
 * class. The struct chosen decides for which plane the calculation needs to be
 * done. If you wish to substract the baseline for all 3 planes call this
 * function 3 times with a different parameter each time.
 * @return int Error codes
 */
template <typename pI> int cleanUVW::substractBl(bool smooth_opt) {

    if (m_uvw_vec.size() == 0)
        return -3;

    // First we smooth the signal.
    for (auto &data_el : m_uvw_vec) {

        if (data_el.plane_val != pI::plane_nr)
            continue;

        // smoothChannel(data_el.signal_val);

        if (smooth_opt) {

            data_el.signal_val = savitzkyGolayFilter(data_el.signal_val);
        }

        // Set the final 12 elements to 0 because of artifacts. The first few
        // elements should also be considered for removal.
        std::fill(data_el.signal_val.begin(), data_el.signal_val.begin() + 10,
                  0);

        std::fill(data_el.signal_val.begin() + 500,
                  data_el.signal_val.begin() + 512, 0);
    }

    // Calculate the charge histogram.
    calculateChargeHist<pI>();

    // Calculate the baseline as the smallest non 0 element in the charge
    // histogram.
    calculateBaseline<pI>();

    // Extract the baseline from the charge histogram.
    std::transform(
        m_charge_val[pI::plane_nr].begin(), m_charge_val[pI::plane_nr].end(),
        m_charge_val[pI::plane_nr].begin(), [this](const double &sig_el) {
            return std::max(0.0, (sig_el - this->m_baseline));
        });

    // Divide the baseline by the size of the plane. We do this because the
    // charge histogram is the added elements in each bin.
    m_baseline /= pI::plane_size;

    for (auto &data_el : m_uvw_vec) {

        if (data_el.plane_val != pI::plane_nr)
            continue;

        data_el.baseline_val = m_baseline;

        // Extract the baseline from the signal. Make any elements lower
        // than 10 equal to 10 so we don't have negative signal values.
        // I didn't make them 0 because I have a problem with empty vectors.
        std::transform(data_el.signal_val.begin(), data_el.signal_val.end(),
                       data_el.signal_val.begin(),
                       [this](const double &sig_el) {
                           return std::max(10.0, (sig_el - this->m_baseline));
                       });
    }

    // double smoothing is probably unnecessary but it's an idea.
    /* for (auto &data_el : m_uvw_vec) {

        if (data_el.plane_val != pI::plane_nr)
            continue;

        // smoothChannel(data_el.signal_val);
        data_el.signal_val = savitzkyGolayFilter(data_el.signal_val);
    }
 */
    return 0;
}

/**
 * @brief Obtains the charge histogram computed in the substractBl function.
 *
 * @tparam pI This parameter should be one of the three structs defined in the
 * class. The struct chosen decides for which plane the calculation needs to be
 * done. If you wish to sget the histogram for all 3 planes call this
 * function 3 times with a different parameter each time.
 * @param charge_hist The charge histogram that you want to get. You only need a
 * TH1D* variable, the object is created inside this function using new TH1D.
 * @return int Error codes.
 */
template <typename pI> int cleanUVW::getChargeHist(TH1D *&charge_hist) {

    if (std::all_of(m_charge_val[pI::plane_nr].begin(),
                    m_charge_val[pI::plane_nr].end(),
                    [](double d) { return d == 0.0; }))
        return -3;

    charge_hist =
        new TH1D(pI::plane_hist_name.c_str(), "Charge histogram", 512, 1, 512);

    charge_hist->SetContent(m_charge_val[pI::plane_nr].data());

    return 0;
}

/**
 * @brief Smooths the signal from a specific channel using the rolling average
 * agorithm. DEPRECATED and no longer used.
 *
 * @param v The vector containing the signal from 1 channel.
 */
void cleanUVW::smoothChannel(std::vector<double> &v) {

    static const int window_size = 8;

    for (auto i = 0; i < v.size(); i++) {

        int start =
            std::max(0, i - window_size / 2);   // Start index of the window
        int end = std::min(int(v.size()) - 1,
                           i + window_size / 2);   // End index of the window

        v[i] = std::accumulate(&v.at(start), &v.at(end), 0.0) /
               (end - start);   // Average of the elements in the window
    }
}

/**
 * @brief Calculates the baseline from the m_charge_val vector. Must be used
 * AFTER this vector is calculated. The vector is calculated in
 * calculateChargeHist().
 * OLD: The baseline is the smallest non 0 element of the vector.
 * NEW: The baseline is the mean of the first 10 elements.
 * If you want to use it for baseline extraction from the actual signals
 * remember to divide by the number of channels on that plane.
 *
 */
template <typename pI> void cleanUVW::calculateBaseline() {

    auto start_iter = std::next(m_charge_val[pI::plane_nr].begin(), 5);
    m_baseline = *std::min_element(
        start_iter, m_charge_val[pI::plane_nr].end(), [](double a, double b) {
            return (a > 0 && b > 0) ? (a < b) : (a > b);
        });

    /* static constexpr int sample_area = 10;

    m_baseline = std::accumulate(m_charge_val.begin(),
                                 m_charge_val.begin() + sample_area, 0) /
                 sample_area; */
}

/**
 * @brief Calculates the charge histogram for 1 plane. The charge histogram is
 * the sum of the n-th bin for each channel.
 * So it's ch0[0] + ch1[0] + ...
 *
 * @tparam pI This parameter should be one of the three structs defined in the
 * class. The struct chosen decides for which plane the calculation needs to be
 * done.
 */
template <typename pI> void cleanUVW::calculateChargeHist() {

    std::vector<dataUVW> filtered_data;
    std::copy_if(
        m_uvw_vec.begin(), m_uvw_vec.end(), std::back_inserter(filtered_data),
        [](const dataUVW &data) { return data.plane_val == pI::plane_nr; });

    m_charge_val[pI::plane_nr].fill(0);

    for (std::size_t i = 0; i < m_charge_val[pI::plane_nr].size(); i++) {
        m_charge_val[pI::plane_nr][i] =
            std::accumulate(filtered_data.begin(), filtered_data.end(), 0.0,
                            [i](double acc, const dataUVW &el) {
                                return acc + el.signal_val[i];
                            });
    }
}

/**
 * @brief Computes the Savitzky-Golay coefficients for the given window size and
 * polynomial order. Must always be called in the constructor.
 *
 */
void cleanUVW::computeSavitzkyGolayCoefficients() {
    // Compute the denominator of the formula
    double denom = 0;
    for (int i = -m_window_size / 2; i <= m_window_size / 2; i++) {
        denom += pow(i, 2 * m_poly_order);
    }
    // Compute the coefficients for each data point in the window
    for (int i = -m_window_size / 2; i <= m_window_size / 2; i++) {
        double num = 0;
        for (int j = 0; j <= m_poly_order; j++) {
            num += pow(i, j) * pow(-1, j) * tgamma(2 * m_poly_order + 1) /
                   (tgamma(j + 1) * tgamma(2 * m_poly_order - j + 1));
        }
        m_coefficients[i + m_window_size / 2] = num / denom;
    }
}

/**
 * @brief Applies a Savitzky-Golay filter to the given signal. The window size
 * and order are constants declared int he class. The values right now are 8
 * and 2.
 * @todo Optimization. Right now a different vector is used for the smoothed
 * signal and is returned. Trey to find a way to only use 1 vector.
 *
 * @param signal The signal on which the filter is applied.
 * @return std::vector<double> The smoothed signal.
 */
std::vector<double>
cleanUVW::savitzkyGolayFilter(const std::vector<double> &signal) {

    std::vector<double> smoothed_signal(signal.size());

    // Apply the filter to each data point in the signal
    for (int i = m_window_size / 2; i < signal.size() - m_window_size / 2;
         i++) {
        double sum = 0;
        for (int j = -m_window_size / 2; j <= m_window_size / 2; j++) {
            sum += m_coefficients[j + m_window_size / 2] * signal[i + j];
        }
        smoothed_signal[i] = sum;
    }

    // return std::move(smoothed_signal);
    // Move may hinder copy elision so it's probably not needed.
    return smoothed_signal;
}

/**
 * @brief Returns the vector containing the vector in the UVW format.
 *
 * @return std::vector<dataUVW> The vector with the data in the UVW format.
 */
std::vector<dataUVW> cleanUVW::returnDataUVW() const { return m_uvw_vec; }
