#ifndef convertXYZ_hpp
#define convertXYZ_hpp 1

#define _USE_MATH_DEFINES
#include <cmath>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#ifndef COS_PIOVER6
#define COS_PIOVER6 (0.866)
#endif

#ifndef TAN_PIOVER6
#define TAN_PIOVER6 (0.577)
#define TAN_MINUSPIOVER6 (-0.577)
#endif

#include "TGraph.h"
#include "TGraph2D.h"

#include "dataUVW.hpp"
#include "dataXYZ.hpp"
#include "hitPoints.hpp"

/**
 * @brief Makes the conversion to the XYZ coordinate system. Takes the
 * information about the hits and returns the XYZ location of the hits.
 *
 */
class convertXYZ {
  public:
    convertXYZ(){};
    ~convertXYZ(){};
    convertXYZ(std::vector<hitPoints> hit_data);

    int makeConversionXYZ();

    std::vector<dataXYZ> returnXYZ();

    int getNewVector(std::vector<hitPoints> hit_data);

  private:
    /// @brief The drift velocity taken from the documentation.
    static constexpr double drift_vel = 0.724 * 1e7;   // 10000 * 1000

    /// @brief The time unit taken from the documentation.
    static constexpr double time_unit = 4e-8;

    /// @brief Vector containing the hits. Taken as input.
    std::vector<hitPoints> m_hit_data;

    /// @brief Vector containing the XYZ coordinates of the hits. Is the output.
    std::vector<dataXYZ> m_points_xyz;

    /// @brief Used for debug.
    bool m_verbose = false;

    std::mutex m_points_xyz_mutex;

    void sortHitData();

    std::pair<double, double> calculateXYfromUV(const int strip_u,
                                                const int strip_v);

    std::pair<double, double> calculateXYfromVW(const int strip_v,
                                                const int strip_w);

    std::pair<double, double> calculateXYfromUW(const int strip_u,
                                                const int strip_w);

    int evaluatePointsEquality(const std::pair<double, double> xy_from_uv,
                               const std::pair<double, double> xy_from_vw,
                               const std::pair<double, double> xy_from_uw);

    void calculateXYZ();
    void calculateXYZ_threaded();

    void processHitData(const hitPoints &hit_u, const hitPoints &hit_v,
                        const hitPoints &hit_w);

    void splitVectorOperation(std::vector<hitPoints>::iterator start_u,
                              std::vector<hitPoints>::iterator end_u,
                              std::vector<hitPoints>::iterator start_v,
                              std::vector<hitPoints>::iterator end_v,
                              std::vector<hitPoints>::iterator start_w,
                              std::vector<hitPoints>::iterator end_w);

    /*
        std::pair<double, double> calculateXYfromUV_V2(int strip_u, int
       strip_v); std::pair<double, double> calculateXYfromVW_V2(int strip_v, int
       strip_w);
        std::pair<double, double> calculateXYfromUW_V2(int strip_u, int
       strip_w); */
};

#endif