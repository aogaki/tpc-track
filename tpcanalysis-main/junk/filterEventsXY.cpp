#include "../include/filterEventsXY.hpp"
/**
 * @brief Filters the event by checking it's size and removes events with too
 * few data points. Afterwards, it pushes the event into the event vector.
 * Each event represents an entry from the tree.
 * Also filters vertical lines, since they have to be treated separately.
 *
 * @param event_entry The event to be added.
 * @return int Error codes.
 */
int filterEventsXY::filterAndPush(generalDataStorage &event_entry) {

    auto vec_size = event_entry.xyz_data.size();

    if (vec_size == 0)
        return -3;

    if (vec_size > (min_event_size - 1) &&
        event_entry.contains_vertical_line != 1)
        m_event_vec.push_back(event_entry);

    return 0;
}

int filterEventsXY::filterAndPush(generalDataStorage &&event_entry) noexcept {

    auto vec_size = event_entry.xyz_data.size();

    if (vec_size == 0)
        return -3;

    if (vec_size > (min_event_size - 1) &&
        event_entry.contains_vertical_line != 1)
        m_event_vec.push_back(std::move(event_entry));

    return 0;
}

int filterEventsXY::resetVector() {

    if (m_event_vec.size() == 0)
        return -3;

    std::vector<generalDataStorage>().swap(m_event_vec);

    return 0;
}

/**
 * @brief Uses linear regression and the Mean Squared Error to split the events
 * in different classes. The classes should be based on how well the linear
 * model fits the data set.
 * The label 1 means the model fits appropriatedly and the label 2 means the
 * model doesn't fit.
 *
 * @return int Error codes.
 */
int filterEventsXY::assignClass() {
    if (m_event_vec.size() == 0)
        return -3;

    // For each event calculate the coefficients m and b from the equation y =
    // m*x + b. Then we compute the MSE and we assign a class based on the value
    // of the MSE. For now, the class 1 means the linear regression model fits
    // well, while the class 2 means that the linear regression model doesnt
    // fit.
    for (auto &event : m_event_vec) {

        double coeff_m = 0;
        double coeff_b = 0;
        double mse_value = 0;

        std::tie(coeff_m, coeff_b) = calculateLinRegCoeff(event.xyz_data);
        mse_value = calculateMSE(event.xyz_data, coeff_m, coeff_b);
        if (mse_value < mse_limit) {

            event.filter_label = 1;
        } else {
            event.filter_label = 2;
        }

        event.mse_value = mse_value;
    }

    return 0;
}

/**
 * @brief Uses linear regression and the Mean Squared Error to split the events
 * in different classes. The classes should be based on how well the linear
 * model fits the data set. It does the same thing as assignClass, only it uses
 * multi threading in hopes of achieving better performance.
 * The label 1 means the model fits appropriatedly and the label 2 means the
 * model doesn't fit.
 *
 * @return int Error codes.
 */
int filterEventsXY::assignClass_threaded() {

    if (m_event_vec.size() == 0)
        return -3;

    unsigned int numThreads = std::thread::hardware_concurrency();

    int segmentSize = (m_event_vec.end() - m_event_vec.begin()) / numThreads;

    std::vector<std::thread> threads;
    for (auto i = 0; i < (numThreads - 1); i++) {
        threads.emplace_back(&filterEventsXY::assignClassToEvent, this,
                             m_event_vec.begin() + i * segmentSize,
                             m_event_vec.begin() + (i + 1) * segmentSize);
    }
    threads.emplace_back(&filterEventsXY::assignClassToEvent, this,
                         m_event_vec.begin() + (numThreads - 1) * segmentSize,
                         m_event_vec.end());

    // Join the threads after they finish executing.
    for (auto &thread : threads) {
        thread.join();
    }

    return 0;
}

/**
 * @brief Assigns a class to a number of events. Is used by
 * assignClass_threaded. The vector containing all of the vents is split into
 * different parts based on the number of threads and each part is worked on by
 * this function.
 *
 * @param start_it The start of the part of the vector to be analyzed.
 * @param end_it The end of the part of the vector to be analyzed.
 */
void filterEventsXY::assignClassToEvent(
    std::vector<generalDataStorage>::iterator start_it,
    std::vector<generalDataStorage>::iterator end_it) {

    for (auto it = start_it; it < end_it; it++) {

        double coeff_m = 0;
        double coeff_b = 0;
        double mse_value = 0;

        std::tie(coeff_m, coeff_b) = calculateLinRegCoeff((*it).xyz_data);
        mse_value = calculateMSE((*it).xyz_data, coeff_m, coeff_b);
        if (mse_value < mse_limit) {

            (*it).filter_label = 1;
        } else {
            (*it).filter_label = 2;
        }

        (*it).mse_value = mse_value;
    }
}

/**
 * @brief Computes the coefficients of the linear model using the linear
 * regression algorithm. The linear model has the equation m*x + b = y.
 *
 * @param curr_event The current event to be analyzed.
 * @return std::tuple<double, double> A tuple of form {coeff_m, coeff_b}.
 */
std::tuple<double, double>
filterEventsXY::calculateLinRegCoeff(const std::vector<dataXYZ> &curr_event) {

    double coeff_m;
    double coeff_b;

    auto vec_size = curr_event.size();

    // Calculate the means of x and y.
    double x_mean = std::accumulate(curr_event.begin(), curr_event.end(), 0.0,
                                    [](const double &acc, const dataXYZ &val) {
                                        return acc + val.data_x;
                                    }) /
                    vec_size;

    double y_mean = std::accumulate(curr_event.begin(), curr_event.end(), 0.0,
                                    [](const double &acc, const dataXYZ &val) {
                                        return acc + val.data_y;
                                    }) /
                    vec_size;

    // The numerator and denominator for the m coefficient.
    double numerator = std::accumulate(
        curr_event.begin(), curr_event.end(), 0.0,
        [x_mean, y_mean](const double &acc, const dataXYZ &val) {
            return acc + (val.data_x - x_mean) * (val.data_y - y_mean);
        });

    double denominator = std::accumulate(
        curr_event.begin(), curr_event.end(), 0.0,
        [x_mean, y_mean](const double &acc, const dataXYZ &val) {
            return acc + (val.data_x - x_mean) * (val.data_x - x_mean);
        });

    coeff_m = numerator / denominator;
    coeff_b = y_mean - coeff_m * x_mean;

    return {coeff_m, coeff_b};
}

/**
 * @brief Calculates the Mean Squared error that should be used to check how
 * well the linear model fith the data set.
 *
 * @param curr_event The event to be analyzed.
 * @param coeff_m The m coefficient.
 * @param coeff_b The b coefficient.
 * @return double The computed value of the MSE.
 */
double filterEventsXY::calculateMSE(const std::vector<dataXYZ> &curr_event,
                                    const double &coeff_m,
                                    const double &coeff_b) {

    double mse_value;

    double sum_squared_diff = std::accumulate(
        curr_event.begin(), curr_event.end(), 0.0,
        [coeff_m, coeff_b](const double &acc, const dataXYZ &val) {
            double y_pred = coeff_m * val.data_x + coeff_b;
            return acc + (y_pred - val.data_y) * (y_pred - val.data_y);
        });

    mse_value = sum_squared_diff / curr_event.size();

    return mse_value;
}

/**
 * @brief Returns the vector containing all of the events and their labels.
 * This function copies the vector so it might be slower.
 *
 * @return std::vector<generalDataStorage> The vector
 * containing the events.
 */
std::vector<generalDataStorage> filterEventsXY::returnEventVector() {
    return m_event_vec;
}
