#include "../include/convertXYZ.hpp"

/**
 * @brief Construct a new convert X Y Z::convert X Y Z object
 *
 * @param hit_data the hitPoints type vector that is converted using the other
 * functions
 */
convertXYZ::convertXYZ(std::vector<hitPoints> hit_data) {
    m_hit_data = std::move(hit_data);
}

/**
 * @brief Get a new vector that is to be converted by the other functions. The
 * old vector is lost.
 *
 * @param hit_data the new vector to be used
 * @return int error codes
 */
int convertXYZ::getNewVector(std::vector<hitPoints> hit_data) {

    std::vector<hitPoints>().swap(m_hit_data);

    std::vector<dataXYZ>().swap(m_points_xyz);

    if (hit_data.size() == 0)
        return -3;

    m_hit_data = std::move(hit_data);

    return 0;
}

/**
 * @brief Makes the conversion from UVW planes to XYZ coordinate system.
 *
 * @return int error codes
 */
int convertXYZ::makeConversionXYZ() {

    if (m_hit_data.size() == 0)
        return -3;

    sortHitData();

    // calculateXYZ();

    calculateXYZ_threaded();

    return 0;
}

/**
 * @brief Sorts the hit data based on the plane value.
 * OLD: Sorts the data based on the position of the peak on the x axis. Right
 * now it is not useful.
 *
 */
void convertXYZ::sortHitData() {

    /* struct{
        bool operator()(hitPoints &a, hitPoints &b)const{return a.peak_x <
    b.peak_x;} }lessHPI; */

    // std::sort(m_hit_data.begin(), m_hit_data.end(), lessHPI);

    std::sort(m_hit_data.begin(), m_hit_data.end(),
              [](const hitPoints &a, const hitPoints &b) {
                  return a.plane < b.plane;
              });
}

/**
 * @brief This function does the actual conversion.
 * First we iterate through all the hits first for U, then for V, and finally
 * for W. We check if the hits happen at the same time. If yes, we calculate X
 * and Y using 3 methods and if the values are the same We calculate the Z value
 * from the time information and add the point to our converted vector.
 *
 */
void convertXYZ::calculateXYZ() {

    // clear the vector
    std::vector<dataXYZ>().swap(m_points_xyz);

    auto start_u = std::lower_bound(
        m_hit_data.begin(), m_hit_data.end(), 0,
        [](const hitPoints &hp, const int v) { return hp.plane < v; });
    auto end_u = std::upper_bound(
        m_hit_data.begin(), m_hit_data.end(), 0,
        [](const int v, const hitPoints &hp) { return v < hp.plane; });

    auto start_v = std::lower_bound(
        m_hit_data.begin(), m_hit_data.end(), 1,
        [](const hitPoints &hp, const int v) { return hp.plane < v; });
    auto end_v = std::upper_bound(
        m_hit_data.begin(), m_hit_data.end(), 1,
        [](const int v, const hitPoints &hp) { return v < hp.plane; });

    auto start_w = std::lower_bound(
        m_hit_data.begin(), m_hit_data.end(), 2,
        [](const hitPoints &hp, const int v) { return hp.plane < v; });
    auto end_w = std::upper_bound(
        m_hit_data.begin(), m_hit_data.end(), 2,
        [](const int v, const hitPoints &hp) { return v < hp.plane; });

    for (auto it_u = start_u; it_u != end_u; ++it_u) {

        if (it_u == m_hit_data.end()) {

            if (m_verbose) {
                std::cout << "Iterator it_u hit the end of the vector."
                          << std::endl;
            }
            break;
        }

        for (auto it_v = start_v; it_v != end_v; ++it_v) {

            if (it_u == m_hit_data.end()) {

                if (m_verbose) {
                    std::cout << "Iterator it_v hit the end of the vector."
                              << std::endl;
                }
                break;
            }

            // see if the peaks are at the same location, continue if not
            if (((*it_v).peak_x < ((*it_u).peak_x - (*it_u).fwhm / 2.355)) ||
                ((*it_v).peak_x > ((*it_u).peak_x + (*it_u).fwhm / 2.355))) {

                continue;
            }

            for (auto it_w = start_w; it_v != end_w; ++it_w) {

                if (it_w == m_hit_data.end()) {

                    if (m_verbose) {
                        std::cout << "Iterator it_w hit the end of the vector."
                                  << std::endl;
                    }
                    break;
                }

                // see if this hit is also at the same location
                if (((*it_w).peak_x <
                     ((*it_v).peak_x - (*it_v).fwhm / 2.355)) ||
                    ((*it_w).peak_x >
                     ((*it_v).peak_x + (*it_v).fwhm / 2.355))) {

                    continue;
                }

                processHitData((*it_u), (*it_v), (*it_w));
            }
        }
    }
}

/**
 * @brief This function does the actual conversion.
 * First we iterate through all the hits first for U, then for V, and finally
 * for W. We check if the hits happen at the same time. If yes, we calculate X
 * and Y using 3 methods and if the values are the same. If the values are the
 * same, it means that the 3 strips the hits were detected on intersect in one
 * point. This point represent the real location of the hit in XY coordintaes.
 * We calculate the Z value from the time information and add the point to our
 * converted vector. The only difference from convertXYZ::calculateXYZ() is that
 * this function uses multi-threading in hopes of being more efficient.
 *
 */
void convertXYZ::calculateXYZ_threaded() {

    // Use this to find out the number of supported threads.
    unsigned int numThreads = std::thread::hardware_concurrency();

    // clear the vector
    std::vector<dataXYZ>().swap(m_points_xyz);

    // The vectors have been ordered based on their planes. We use this to find
    // out where the hits on each plane start and end. This way we don't need to
    // crate a new vector.
    auto start_u = std::lower_bound(
        m_hit_data.begin(), m_hit_data.end(), 0,
        [](const hitPoints &hp, const int v) { return hp.plane < v; });
    auto end_u = std::upper_bound(
        m_hit_data.begin(), m_hit_data.end(), 0,
        [](const int v, const hitPoints &hp) { return v < hp.plane; });

    auto start_v = std::lower_bound(
        m_hit_data.begin(), m_hit_data.end(), 1,
        [](const hitPoints &hp, const int v) { return hp.plane < v; });
    auto end_v = std::upper_bound(
        m_hit_data.begin(), m_hit_data.end(), 1,
        [](const int v, const hitPoints &hp) { return v < hp.plane; });

    auto start_w = std::lower_bound(
        m_hit_data.begin(), m_hit_data.end(), 2,
        [](const hitPoints &hp, const int v) { return hp.plane < v; });
    auto end_w = std::upper_bound(
        m_hit_data.begin(), m_hit_data.end(), 2,
        [](const int v, const hitPoints &hp) { return v < hp.plane; });

    // Calculate the size of each segment based on the number of supported
    // threads. We then split the vector is segments of this size.
    int segmentSize = (end_u - start_u) / numThreads;

    // Create the threads and pass the iterators for each segment to the
    // splitVectorOperation function.
    std::vector<std::thread> threads;
    for (auto i = 0; i < (numThreads - 1); i++) {
        threads.emplace_back(
            &convertXYZ::splitVectorOperation, this, start_u + i * segmentSize,
            start_u + (i + 1) * segmentSize, start_v, end_v, start_w, end_w);
    }
    threads.emplace_back(&convertXYZ::splitVectorOperation, this,
                         start_u + (numThreads - 1) * segmentSize, end_u,
                         start_v, end_v, start_w, end_w);

    // Join the threads after they finish executing.
    for (auto &thread : threads) {
        thread.join();
    }
}

/**
 * @brief
 *
 * @param start_u
 * @param end_u
 * @param start_v
 * @param end_v
 * @param start_w
 * @param end_w
 */
void convertXYZ::splitVectorOperation(std::vector<hitPoints>::iterator start_u,
                                      std::vector<hitPoints>::iterator end_u,
                                      std::vector<hitPoints>::iterator start_v,
                                      std::vector<hitPoints>::iterator end_v,
                                      std::vector<hitPoints>::iterator start_w,
                                      std::vector<hitPoints>::iterator end_w) {

    for (auto it_u = start_u; it_u != end_u; ++it_u) {

        if (it_u == m_hit_data.end()) {

            if (m_verbose) {
                std::cout << "Iterator it_u hit the end of the vector."
                          << std::endl;
            }
            break;
        }

        for (auto it_v = start_v; it_v != end_v; ++it_v) {

            if (it_u == m_hit_data.end()) {

                if (m_verbose) {
                    std::cout << "Iterator it_v hit the end of the vector."
                              << std::endl;
                }
                break;
            }

            // see if the peaks are at the same location, continue if not
            if (((*it_v).peak_x < ((*it_u).peak_x - (*it_u).fwhm / 2.355)) ||
                ((*it_v).peak_x > ((*it_u).peak_x + (*it_u).fwhm / 2.355))) {

                continue;
            }

            for (auto it_w = start_w; it_v != end_w; ++it_w) {

                if (it_w == m_hit_data.end()) {

                    if (m_verbose) {
                        std::cout << "Iterator it_w hit the end of the vector."
                                  << std::endl;
                    }
                    break;
                }

                // see if this hit is also at the same location
                if (((*it_w).peak_x <
                     ((*it_v).peak_x - (*it_v).fwhm / 2.355)) ||
                    ((*it_w).peak_x >
                     ((*it_v).peak_x + (*it_v).fwhm / 2.355))) {

                    continue;
                }

                processHitData((*it_u), (*it_v), (*it_w));
            }
        }
    }
}

/**
 * @brief Computes the X and Y values from the U and V plane hits.
 *
 * @param strip_u the strip values from the U plane
 * @param strip_v the strip values from the V plane
 * @return std::pair<double, double> the calculated X and Y values; the first
 * element is X and the second is Y
 */
std::pair<double, double> convertXYZ::calculateXYfromUV(const int strip_u,
                                                        const int strip_v) {

    std::pair<double, double> xy_from_uv;

    // 0.866 is cos(M_PI/6)
    double y_part_from_v = 4.33 + (strip_v - 1) * (1.5 / COS_PIOVER6);

    // 106.5 is the width on x of the plate. 1.5 is the distance between strips
    // since the strips are between 1 and 72 we substract 1 so the first strip
    // is at 106.5 and the last is at 0
    xy_from_uv.first = 106.5 - ((strip_u - 1) * 1.5);

    //-0.577 is tan(-M_PI/6) which is -tan(M_PI/6)
    xy_from_uv.second = TAN_MINUSPIOVER6 * xy_from_uv.first + y_part_from_v;

    return xy_from_uv;
}

/**
 * @brief  Computes the X and Y values from the V and W plane hits.
 *
 * @param strip_v the strip values from the V plane
 * @param strip_w the strip values from the W plane
 * @return std::pair<double, double> the calculated X and Y values; the first
 * element is X and the second is Y
 */
std::pair<double, double> convertXYZ::calculateXYfromVW(const int strip_v,
                                                        const int strip_w) {

    std::pair<double, double> xy_from_vw;

    // 0.866 is cos(M_PI/6)
    double part_from_v = 4.33 + (strip_v - 1) * (1.5 / COS_PIOVER6);
    double part_from_w = 100.459 - (strip_w - 1) * (1.5 / COS_PIOVER6);

    // 0.577 is tan(M_PI/6) and -0.577 is tan(-M_PI/6)
    xy_from_vw.first =
        (part_from_w - part_from_v) / (TAN_MINUSPIOVER6 - TAN_PIOVER6);
    xy_from_vw.second = (-TAN_PIOVER6) * xy_from_vw.first + part_from_v;

    return xy_from_vw;
}

/**
 * @brief  Computes the X and Y values from the U and W plane hits.
 *
 * @param strip_u the strip values from the U plane
 * @param strip_w the strip values from the W plane
 * @return std::pair<double, double> the calculated X and Y values; the first
 * element is X and the second is Y
 */
std::pair<double, double> convertXYZ::calculateXYfromUW(const int strip_u,
                                                        const int strip_w) {

    std::pair<double, double> xy_from_uw;

    // 0.866 is cos(M_PI/6) and cos(-M_PI/6)
    double y_part_from_w = 100.459 - (strip_w - 1) * (1.5 / COS_PIOVER6);

    xy_from_uw.first = 106.5 - ((strip_u - 1) * 1.5);

    // 0.577 is tan(M_PI/6)
    xy_from_uw.second = TAN_PIOVER6 * xy_from_uw.first + y_part_from_w;

    return xy_from_uw;
}

/**
 * @brief This function checks to see if the 3 X and Y pairs calculated from the
 * data of 2 planes are almost equal. The calibration variable is used to set
 * the range of the equality.
 *
 * @param xy_from_uv x and y calculated from the u and v plane data
 * @param xy_from_vw x and y calculated from the v and w plane data
 * @param xy_from_uw x and y calculated from the u and w plane data
 * @return int 1 if true 0 if false
 */
int convertXYZ::evaluatePointsEquality(
    const std::pair<double, double> xy_from_uv,
    const std::pair<double, double> xy_from_vw,
    const std::pair<double, double> xy_from_uw) {

    const double calib_variable = 3;

    /* return (xy_from_uv.first > (xy_from_vw.first - calib_variable) &&
       xy_from_uv.first < (xy_from_vw.first + calib_variable) &&
        xy_from_uv.second > (xy_from_vw.second - calib_variable) &&
       xy_from_uv.second < (xy_from_vw.second + calib_variable) &&
        xy_from_uv.first > (xy_from_uw.first - calib_variable) &&
       xy_from_uv.first < (xy_from_uw.first + calib_variable) &&
        xy_from_uv.second > (xy_from_uw.second - calib_variable) &&
       xy_from_uv.second < (xy_from_uw.second + calib_variable) &&
        //xy_from_vw.first > (xy_from_uw.first - calib_variable) &&
       xy_from_vw.first < (xy_from_uw.first + calib_variable) &&
        //xy_from_vw.second > (xy_from_uw.second - calib_variable) &&
       xy_from_vw.second < (xy_from_uw.second + calib_variable) &&
        xy_from_uv.first > -3 && xy_from_uv.first < 110 && xy_from_uv.second >
       -3 && xy_from_uv.second < 110) ? 1 : 0; */

    return (abs(xy_from_uv.first - xy_from_vw.first) < calib_variable &&
            abs(xy_from_uv.second - xy_from_vw.second) < calib_variable &&
            abs(xy_from_uv.first - xy_from_uw.first) < calib_variable &&
            abs(xy_from_uv.second - xy_from_uw.second) < calib_variable &&
            xy_from_uv.first > -3 && xy_from_uv.first < 110 &&
            xy_from_uv.second > -3 && xy_from_uv.second < 110)
               ? 1
               : 0;
}

/**
 * @brief Returns the vector contining the x, y and z values calculated.
 *
 * @return std::vector<dataXYZ> the vector containing the x, y and z values of
 * the points.
 */
std::vector<dataXYZ> convertXYZ::returnXYZ() { return m_points_xyz; }

/**
 * @brief Using three simultaneous hits, this funcion calculates if the strips
 * the hits were detected on intersect. If yes, then we use the intersection
 * point as the real (as in XY physical coordinates) life location of the hits.
 *
 * @param hit_u The hit on the U plane.
 * @param hit_v The hit on the U plane.
 * @param hit_w The hit on the U plane.
 */
void convertXYZ::processHitData(const hitPoints &hit_u, const hitPoints &hit_v,
                                const hitPoints &hit_w) {

    std::pair<double, double> xy_from_uv;
    std::pair<double, double> xy_from_vw;
    std::pair<double, double> xy_from_uw;

    xy_from_uv = calculateXYfromUV(hit_u.strip, hit_v.strip);
    xy_from_vw = calculateXYfromVW(hit_v.strip, hit_w.strip);
    xy_from_uw = calculateXYfromUW(hit_u.strip, hit_w.strip);

    if (evaluatePointsEquality(xy_from_uv, xy_from_vw, xy_from_uw)) {

        dataXYZ loc_xyz;
        loc_xyz.data_x =
            (xy_from_uv.first + xy_from_vw.first + xy_from_uw.first) / 3;
        loc_xyz.data_y =
            (xy_from_uv.second + xy_from_vw.second + xy_from_uw.second) / 3;
        loc_xyz.data_z = drift_vel * time_unit * hit_u.peak_x;
        loc_xyz.data_charge = hit_u.peak_y + hit_v.peak_y + hit_w.peak_y +
                              hit_u.base_line + hit_v.base_line +
                              hit_w.base_line;

        std::lock_guard<std::mutex> lock(m_points_xyz_mutex);
        m_points_xyz.push_back(loc_xyz);
    }
}