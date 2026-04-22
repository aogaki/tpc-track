#include "../include/convertHitData.hpp"

/**
 * @brief Construct a new convert Hit Data::convert Hit Data object.
 *
 * @param uvw_data the uvw data vector to be used when finding the hits
 */
convertHitData::convertHitData(std::vector<dataUVW> uvw_data) {
    m_uvw_data = std::move(uvw_data);
}

/**
 * @brief Destroy the convert Hit Data::convert Hit Data object.
 * Also deletes the raw histograms in m_raw_hist_data.
 *
 */
convertHitData::~convertHitData() {

    for (auto &&loc_hist : m_raw_hist_data) {

        if (loc_hist) {

            delete (loc_hist);
            loc_hist = nullptr;
        }
    }
}

/**
 * @brief Sets the uvw vector to be used when finding the hits to the new value.
 *
 * @param uvw_data the vector containing the information about the 3 planes
 * @return int error codes
 */
int convertHitData::setUVWData(std::vector<dataUVW> uvw_data) {

    // The size of uvw should always be the same so we can just clear.
    // std::vector<dataUVW>().swap(m_uvw_data);
    m_uvw_data.clear();

    std::vector<hitPoints>().swap(m_hit_data);

    // ROOT should clear the memory automatically for TH1D but I'm not sure.
    std::vector<TH1D *>().swap(m_raw_hist_data);

    if (uvw_data.size() == 0)
        return -3;

    m_uvw_data = std::move(uvw_data);

    return 0;
}

/**
 * @brief Returns the vector of hits.
 *
 * @return std::vector<hitPoints> the vector of hits calculated
 */
std::vector<hitPoints> convertHitData::returnHitData() { return m_hit_data; }

/**
 * @brief Returns the vector of histograms.
 *
 * @return std::vector<TH1D*> the vector of histograms containing the
 * information about the peaks for each strip
 */
std::vector<TH1D *> convertHitData::returnHistData() { return m_raw_hist_data; }

/**
 * @brief gets the hit info and saves the data obtained in a hitData vector and
 * in a histogram vector Firstly, TSpectrum is used to find the number of peaks.
 * We then eliminate the peaks that are below a threshold value (currently 200).
 * A function of form ‘pol0+gaus+gaus+...’ is created based on the numbers of
 * peaks found. We use this function to fit the histograms. The parameters
 * obtained from the histogram fitting are saved in the hitData type vector.
 * OLD: The peak threshold in taken as an input variable.
 * OLD: The peak threshold is 2.5 times the mean of the signal.
 * OLD: The peak threshold is now the maximum value between 2.5 times the mean
 * of the signal and 0.75 times the maximum value of the signal. Might need to
 * be adjusted further.
 * NEW: The peak threshold is now the maximum value between
 * sensitivity_avg times the mean of the signal and sensitivity_max times the
 * maximum value of the signal. Might need to be adjusted further.
 *
 * @param sensitivity_avg The
 * @param sensitivity_max
 * @return int error codes
 */
int convertHitData::getHitInfo(const Double_t sensitivity_avg,
                               const Double_t sensitivity_max) {

    // This makes it so the canvas doesn't open a window every time and disrupt
    // the user.
    gROOT->SetBatch(kTRUE);

    auto loc_canv = new TCanvas("canvas from hit info", "hit info canvas");

    if (m_uvw_data.size() == 0)
        return -3;   // invalid size

    // first create the histograms
    int bin = 0;

    int hist_nr = 1;

    std::vector<double> peak_th_vec;

    //#####################

    std::sort(m_uvw_data.begin(), m_uvw_data.end(),
              [](const dataUVW &a, const dataUVW &b) {
                  return (a.strip_nr + 1000 * a.plane_val) <
                         (b.strip_nr + 1000 * b.plane_val);
              });

    //######################

    for (const auto &iter : m_uvw_data) {

        // Shouldn't be needed.
        /* if (gDirectory->FindObject(Form("entry%d_hist_at_strip%d_plane%d",
                                        iter.entry_nr, iter.strip_nr,
                                        iter.plane_val))) {
            continue;
        } */

        auto loc_hist =
            new TH1D(Form("entry%d_hist_at_strip%d_plane%d", iter.entry_nr,
                          iter.strip_nr, iter.plane_val),
                     Form("hist%d", hist_nr), 512, 1, 512);

        // By setting the content of each bin to a value of the signal we can
        // crate a graph using TH1.
        for (const auto &sig_iter : iter.signal_val) {
            loc_hist->SetBinContent(++bin, sig_iter);
        }

        m_raw_hist_data.push_back(loc_hist);

        double loc_peak_th = std::max(
            sensitivity_max * (*std::max_element(iter.signal_val.begin(),
                                                 iter.signal_val.end())),
            sensitivity_avg * (std::accumulate(iter.signal_val.begin(),
                                               iter.signal_val.end(), 0.0) /
                               iter.signal_val.size()));

        peak_th_vec.push_back(loc_peak_th);

        bin = 0;
        hist_nr++;
    }

    // the calculate the hit data
    /* auto spec_analyzer = new TSpectrum();

    Double_t *pos_holder_x;
    Double_t *pos_holder_y;

    int npeaks; */

    int curr_iter = 0;

    for (const auto &hist_iter : m_raw_hist_data) {

        // find the peaks in the histogram with TSpectrum
        /* spec_analyzer->Search(hist_iter, 5, "nodraw", 0.2);
        npeaks = spec_analyzer->GetNPeaks();
        pos_holder_x = spec_analyzer->GetPositionX();
        pos_holder_y = spec_analyzer->GetPositionY(); */

        // find which peaks are good
        std::vector<double> peaks_x;   // x of valid peaks
        std::vector<double> peaks_y;   // y of valid peaks
        int nrealpeaks = 0;            // number of valid peaks set to 0

        // std::cout << "\nBefore find at " << curr_iter << std::endl;
        std::tie(nrealpeaks, peaks_x, peaks_y) =
            findPeaks(hist_iter, peak_th_vec[curr_iter]);

        // std::cout << "After find at " << curr_iter << std::endl;

        /* for (auto i = 0; i < npeaks; i++) {
            if (pos_holder_y[i] > peak_th_vec[curr_iter]) {
                peaks_x.push_back(pos_holder_x[i]);
                peaks_y.push_back(pos_holder_y[i]);
                nrealpeaks++;
            }
        } */

        // std::cout<<"Peaks found: "<<npeaks<<" of which valid are:
        // "<<peaks_y.size()<<std::endl;

        // Build the function based on the number of valid peaks found. The
        // base function is a 0th degree polinomial.
        TString func_holder("pol0(0)");

        // Add a gaussian to the function for each peak found.
        for (auto i = 0; i < nrealpeaks; i++)
            func_holder.Append(Form("+gaus(%d)", (3 * i + 1)));

        auto gaus_and_pol0 =
            new TF1("gaus_and_pol0", func_holder.Data(), 1, 512);

        // std::cout<<"Function is "<<func_holder.Data()<<std::endl;
        gaus_and_pol0->SetParameter(0, 3);
        for (auto i = 0; i < nrealpeaks; i++) {
            gaus_and_pol0->SetParameter((3 * i + 1), peaks_y[i]);
            gaus_and_pol0->SetParameter((3 * i + 2), peaks_x[i]);
            gaus_and_pol0->SetParameter((3 * i + 3), 10);
            gaus_and_pol0->SetParLimits((3 * i + 3), 0, 1000);
        }

        TFitResultPtr result = hist_iter->Fit("gaus_and_pol0", "Q");

        for (auto i = 0; i < nrealpeaks; i++) {

            hitPoints curr_point;

            curr_point.peak_x = hist_iter->GetFunction("gaus_and_pol0")
                                    ->GetParameter((3 * i + 2));
            curr_point.peak_y = hist_iter->GetFunction("gaus_and_pol0")
                                    ->GetParameter((3 * i + 1));
            curr_point.fwhm = 2.355 * hist_iter->GetFunction("gaus_and_pol0")
                                          ->GetParameter((3 * i + 3));
            curr_point.plane = m_uvw_data[curr_iter].plane_val;
            curr_point.strip = m_uvw_data[curr_iter].strip_nr;
            curr_point.entry_nr = m_uvw_data[curr_iter].entry_nr;
            curr_point.base_line =
                m_uvw_data[curr_iter]
                    .baseline_val;   // this is more relevant and can be used to
                                     // calculate the charge
            // curr_point.base_line =
            // hist_iter->GetFunction("gaus_and_pol0")->GetParameter(0);

            /* curr_point.peak_x = peaks_x[i];
            curr_point.peak_y = peaks_y[i]; */

            m_hit_data.push_back(curr_point);
        }

        if (gaus_and_pol0) {
            delete (gaus_and_pol0);
            gaus_and_pol0 = nullptr;
        }

        curr_iter++;
    }

    /* if (spec_analyzer) {
        delete (spec_analyzer);
        spec_analyzer = nullptr;
    } */

    //#######################

    /* loc_canv->Print("./converteddata/currpdf.pdf[");

    for (const auto &hist_iter : m_raw_hist_data) {

        hist_iter->Draw("COLZ");
        loc_canv->Update();
        loc_canv->Print("./converteddata/currpdf.pdf");
    }

    loc_canv->Print("./converteddata/currpdf.pdf]"); */

    //########################

    if (loc_canv) {
        loc_canv->Close();
        gSystem->ProcessEvents();
        delete (loc_canv);
        loc_canv = nullptr;
    }

    gROOT->SetBatch(kFALSE);

    return 0;
}

/**
 * @brief Alternative to TSpectrum->Search() for finding peaks.
 *
 * @param loc_hist The histogram in which we want to find the peaks.
 * @param peak_th The minimum accepted value of the peak on y.
 * @return std::tuple<int, std::vector<double>, std::vector<double>> The number
 * of detected peaks. X value of the peaks. Y value of the peaks.
 */
std::tuple<int, std::vector<double>, std::vector<double>>
convertHitData::findPeaks(const TH1D *loc_hist, const double &peak_th) {

    std::vector<double> peaks_x;   // x of valid peaks
    std::vector<double> peaks_y;

    // If the peaks are within this value of bins of each other then we consider
    // them part of the same peak.
    static constexpr int same_peak_th = 8;

    int n_peaks = 0;

    // Default 0 value should't create problems if we remember it exists.
    double last_peak_x = 0;
    double last_peak_y = 0;

    for (auto i = 1; i < loc_hist->GetSize() - 1; ++i) {

        auto curr_peak_y = loc_hist->GetBinContent(i);

        // We check if the current value is a local peak.
        if (curr_peak_y > loc_hist->GetBinContent(i - 1) &&
            curr_peak_y > loc_hist->GetBinContent(i + 1) &&
            curr_peak_y > peak_th) {

            // If 2 peaks are close together we consider them part of the same
            // peak and we choose the higher value on y as the true peak.
            // Also make sure we have actual peaks. Otherwise, because
            // last_peak_x is by default 0, then if a peak is close to 0 it it
            // will try to modify an peak that doesn't exist and crash.
            if (n_peaks != 0 && (i - last_peak_x) < same_peak_th) {

                if (curr_peak_y > last_peak_y) {

                    // Better to modify the old then to pop and push new values.
                    peaks_x[n_peaks - 1] = i;
                    peaks_y[n_peaks - 1] = curr_peak_y;

                    // Try catch should't be needed, so I'll just comment it.
                    /* try {

                        peaks_x.at(n_peaks - 1) = i;
                        peaks_y.at(n_peaks - 1) = curr_peak_y;
                    } catch (const std::exception &e) {

                        std::cerr << "Find peaks threw exception " << e.what()
                                  << " at i " << i << " with n peaks "
                                  << n_peaks << std::endl;

                        peaks_x.push_back(i);
                        peaks_y.push_back(curr_peak_y);
                    } */

                } else {
                    continue;
                }
            } else {

                peaks_x.push_back(i);
                peaks_y.push_back(curr_peak_y);
                n_peaks++;
            }

            last_peak_x = i;
            last_peak_y = curr_peak_y;
        }
    }

    return {n_peaks, std::move(peaks_x), std::move(peaks_y)};
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool convertHitData::containsVerticalLine() {

    if (m_hit_data.size() == 0) {

        throw std::invalid_argument("The number of points is 0.");
    }

    static constexpr int min_num_points = 10;

    static constexpr int error_margin = 3;

    std::unordered_map<int, int> countMap;

    for (const auto &point : m_hit_data) {

        /* for (auto i = (point.peak_x - error_margin);
             i < (point.peak_x + error_margin + 1); ++i) {

            countMap[i]++;
            if (countMap[i] > min_num_points) {

                return true;
            }
        } */

        countMap[point.peak_x]++;
        if (countMap[point.peak_x] > min_num_points) {

            return true;
        }
    }

    int maxCount = 0;
    int maxX = 0;

    for (const auto &pair : countMap) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
            maxX = pair.first;
        }
    }

    std::cout << "Maximum element: " << maxX << " with count " << maxCount
              << std::endl;

    return false;
}