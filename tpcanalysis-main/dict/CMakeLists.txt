#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(MyLib)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ROOT >= 6.32 requires C++17
add_definitions("-std=c++17 -march=native -mtune=native")

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

# Bake the absolute build path into the dylib's install_name so dependents
# (ROOT macros in interpreter or ACLiC mode) can locate it at load time
# without needing DYLD_LIBRARY_PATH. Without this, the default install_name
# is "@rpath/libMyLib.dylib" and dlopen can't find it.
set_target_properties(MyLib PROPERTIES
    INSTALL_NAME_DIR "${CMAKE_CURRENT_BINARY_DIR}"
    BUILD_WITH_INSTALL_NAME_DIR TRUE)
