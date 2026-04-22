#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(MyLib)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# I wanna compile with C++11
add_definitions("-std=c++14 -march=native -mtune=native")

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV{ROOTSYS}/ect/cmake")
find_package(ROOT REQUIRED COMPONENTS RIO Net)
include(${ROOT_USE_FILE})

#----------------------------------------------------------------------------
# Locate sources and headers for this project
#
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${ROOT_INCLUDE_DIR})
link_directories(${ROOT_LIBRARY_DIR})
file(GLOB MY_SRCS ${PROJECT_SOURCE_DIR}/src/*)
file(GLOB MY_HEADERS ${PROJECT_SOURCE_DIR}/include/*)

# Generating Dictionary for ROOT
set(MY_LINKDEF LinkDefGET.h )
set(MY_DICTIONARY "G__${PROJECT_NAME}")
ROOT_GENERATE_DICTIONARY(${MY_DICTIONARY} ${MY_HEADERS} LINKDEF ${MY_LINKDEF})

#----------------------------------------------------------------------------
# Generating executable file and linking
#
add_library(MyLib SHARED ${MY_SRCS} "${MY_DICTIONARY}.cxx")
target_link_libraries(MyLib ${ROOT_LIBRARIES})
