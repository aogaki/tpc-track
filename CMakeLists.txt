cmake_minimum_required(VERSION 3.14)
project(tpc_track CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(tpctrack_core
    src/geometry.cpp
    src/hit_extraction.cpp
)
target_include_directories(tpctrack_core PUBLIC include)

enable_testing()

include(FetchContent)
# doctest v2.4.11 uses cmake_minimum_required(VERSION 3.0) which CMake 4 rejects.
# Force a compatible policy version for the fetched project.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

add_executable(test_geometry tests/test_geometry.cpp)
target_link_libraries(test_geometry PRIVATE tpctrack_core doctest::doctest)
target_compile_definitions(test_geometry PRIVATE
    TPCTRACK_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tpcanalysis-main/utils")
add_test(NAME test_geometry COMMAND test_geometry)

add_executable(test_hit_extraction tests/test_hit_extraction.cpp)
target_link_libraries(test_hit_extraction PRIVATE tpctrack_core doctest::doctest)
add_test(NAME test_hit_extraction COMMAND test_hit_extraction)
