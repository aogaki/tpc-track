cmake_minimum_required(VERSION 3.16)
project(tpc_track CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# -O3 is known to cause surprising FP reordering / threshold drift on this
# project. Force -O2 for Release to keep results reproducible.
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)

add_compile_options(-Wall -Wextra -Wpedantic)

# -------------------------------------------------------------------------
# tpctrack_core — our ROOT-free library
# -------------------------------------------------------------------------
add_library(tpctrack_core STATIC
    src/tpctrack/geometry.cpp
    src/tpctrack/hit_extraction.cpp
    src/tpctrack/uvw_xyz.cpp
)
target_include_directories(tpctrack_core PUBLIC include)

# -------------------------------------------------------------------------
# ROOT — needed by the upstream library (.root I/O + GET dictionary)
# -------------------------------------------------------------------------
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree Hist)
include(${ROOT_USE_FILE})

# -------------------------------------------------------------------------
# tpc_upstream — vendored miniTPC reader (loadData / convertUVW / cleanUVW)
# plus the GET dictionary classes. ROOT_GENERATE_DICTIONARY emits
# G__tpc_upstream.cxx, which is compiled into the same library so ROOT I/O
# can deserialise GDataFrame objects stored in the .root files.
# -------------------------------------------------------------------------
set(GET_DICT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/upstream/GDataFrame.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/upstream/GDataChannel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/upstream/GDataSample.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/upstream/GFrameHeader.h
)
ROOT_GENERATE_DICTIONARY(G__tpc_upstream
    ${GET_DICT_HEADERS}
    LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/include/upstream/LinkDefGET.h
    MODULE tpc_upstream
)

add_library(tpc_upstream SHARED
    src/upstream/GDataChannel.cpp
    src/upstream/GDataFrame.cpp
    src/upstream/GDataSample.cpp
    src/upstream/GFrameHeader.cpp
    src/upstream/loadData.cpp
    src/upstream/convertUVW_mini.cpp
    src/upstream/cleanUVW.cpp
    G__tpc_upstream.cxx
)
target_include_directories(tpc_upstream PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/upstream
)
target_link_libraries(tpc_upstream PUBLIC
    ROOT::Core ROOT::RIO ROOT::Tree ROOT::Hist
)
# vendored code has -Wsign-compare warnings we don't want to fix here.
target_compile_options(tpc_upstream PRIVATE -Wno-sign-compare -Wno-unused-parameter)
# Keep the .pcm next to the .so so ROOT can auto-load the dictionary.
set_target_properties(tpc_upstream PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# -------------------------------------------------------------------------
# run_mini — the standalone executable
# -------------------------------------------------------------------------
add_executable(run_mini tools/run_mini_main.cpp)
target_link_libraries(run_mini PRIVATE tpctrack_core tpc_upstream)

# -------------------------------------------------------------------------
# Tests (doctest)
# -------------------------------------------------------------------------
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
    TPCTRACK_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/utils")
add_test(NAME test_geometry COMMAND test_geometry)

add_executable(test_hit_extraction tests/test_hit_extraction.cpp)
target_link_libraries(test_hit_extraction PRIVATE tpctrack_core doctest::doctest)
add_test(NAME test_hit_extraction COMMAND test_hit_extraction)

add_executable(test_uvw_xyz tests/test_uvw_xyz.cpp)
target_link_libraries(test_uvw_xyz PRIVATE tpctrack_core doctest::doctest)
target_compile_definitions(test_uvw_xyz PRIVATE
    TPCTRACK_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/utils")
add_test(NAME test_uvw_xyz COMMAND test_uvw_xyz)
