#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "tpctrack/hit_extraction.hpp"

using tpctrack::extractHits;
using tpctrack::Hit;
using tpctrack::StripSignal;

TEST_CASE("Zero signal yields no hits") {
    std::vector<StripSignal> input{
        {0, 1, std::vector<double>(512, 0.0)},
        {1, 3, std::vector<double>(512, 0.0)},
    };
    auto hits = extractHits(input, 10.0);
    CHECK(hits.empty());
}

TEST_CASE("Single suprathreshold pulse produces hits matching its width") {
    std::vector<double> signal(512, 0.0);
    signal[100] = 50.0;
    signal[101] = 70.0;
    signal[102] = 50.0;

    std::vector<StripSignal> input{{0, 5, signal}};
    auto hits = extractHits(input, 20.0);

    REQUIRE(hits.size() == 3);
    CHECK(hits[0].plane == 0);
    CHECK(hits[0].strip == 5);
    CHECK(hits[0].time_bin == 100);
    CHECK(hits[0].charge == doctest::Approx(50.0));
    CHECK(hits[1].time_bin == 101);
    CHECK(hits[1].charge == doctest::Approx(70.0));
    CHECK(hits[2].time_bin == 102);
    CHECK(hits[2].charge == doctest::Approx(50.0));
}

TEST_CASE("Threshold is strictly greater-than (equal values are rejected)") {
    std::vector<double> signal(512, 0.0);
    signal[10] = 20.0;
    std::vector<StripSignal> input{{2, 9, signal}};

    auto hits_eq = extractHits(input, 20.0);
    CHECK(hits_eq.empty());

    auto hits_lt = extractHits(input, 19.999);
    REQUIRE(hits_lt.size() == 1);
    CHECK(hits_lt[0].time_bin == 10);
    CHECK(hits_lt[0].charge == doctest::Approx(20.0));
}

TEST_CASE("Subthreshold bins are ignored, suprathreshold kept") {
    std::vector<double> signal(512, 0.0);
    signal[50] = 5.0;    // subthreshold
    signal[51] = 30.0;   // suprathreshold
    signal[52] = 8.0;    // subthreshold
    signal[53] = 45.0;   // suprathreshold

    std::vector<StripSignal> input{{1, 7, signal}};
    auto hits = extractHits(input, 10.0);

    REQUIRE(hits.size() == 2);
    CHECK(hits[0].time_bin == 51);
    CHECK(hits[0].charge == doctest::Approx(30.0));
    CHECK(hits[1].time_bin == 53);
    CHECK(hits[1].charge == doctest::Approx(45.0));
}

TEST_CASE("Multiple strips produce hits from all of them") {
    std::vector<double> s1(512, 0.0);
    s1[100] = 50.0;
    std::vector<double> s2(512, 0.0);
    s2[200] = 80.0;
    s2[201] = 80.0;

    std::vector<StripSignal> input{
        {0, 1, s1},
        {1, 2, s2},
    };
    auto hits = extractHits(input, 10.0);

    REQUIRE(hits.size() == 3);
    // Per-strip contents are checked without ordering assumptions across strips.
    int count_u1 = 0, count_v2 = 0;
    for (const auto& h : hits) {
        if (h.plane == 0 && h.strip == 1) ++count_u1;
        if (h.plane == 1 && h.strip == 2) ++count_v2;
    }
    CHECK(count_u1 == 1);
    CHECK(count_v2 == 2);
}

TEST_CASE("Empty input returns no hits") {
    std::vector<StripSignal> input;
    auto hits = extractHits(input, 10.0);
    CHECK(hits.empty());
}
