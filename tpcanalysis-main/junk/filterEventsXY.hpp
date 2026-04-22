#ifndef filterEventsXY
#define filterEventsXY_hpp 1

#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

#include "dataXYZ.hpp"
#include "generalDataStorage.hpp"

class filterEventsXY {
  public:
    filterEventsXY(){};
    ~filterEventsXY(){};

    int filterAndPush(generalDataStorage &event_entry);
    int filterAndPush(generalDataStorage &&event_entry) noexcept;
    int resetVector();
    int assignClass();
    int assignClass_threaded();

    std::vector<generalDataStorage> returnEventVector();

  private:
    /// @brief The vector containing all of the events to be analyzed.
    std::vector<generalDataStorage> m_event_vec;

    /// @brief The minimum size of the event to be added to the vector.
    static constexpr std::size_t min_event_size = 50;

    /// @brief The limit of the MSE value for which the laber 1 is assigned. If
    /// mse < mse_limit then label = 1 , otherwise label = 2.
    static constexpr double mse_limit = 12;

    void assignClassToEvent(std::vector<generalDataStorage>::iterator start_it,
                            std::vector<generalDataStorage>::iterator end_it);
    std::tuple<double, double>
    calculateLinRegCoeff(const std::vector<dataXYZ> &curr_event);
    double calculateMSE(const std::vector<dataXYZ> &curr_event,
                        const double &coeff_m, const double &coeff_b);
};

#endif