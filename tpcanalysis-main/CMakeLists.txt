cmake_minimum_required(VERSION 3.0)

set(project_name tpcanalexec)
project(${project_name})

message("${project_name}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -fPIC")
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT REQUIRED COMPONENTS RIO Net)
include(${ROOT_USE_FILE})

include_directories(${PROJECT_SOURCE_DIR}
                    ${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/dict/include
                    ${ROOT_INCLUDE_DIR})

file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hpp ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/dict/include/*.h)
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/dict/src/*.cpp)

set(exe_name "tpcanalexec")

add_executable(${exe_name} main.cpp ${sources})
target_include_directories(${exe_name} PRIVATE
                           ${PROJECT_SOURCE_DIR}/include
                           ${PROJECT_SOURCE_DIR}/dict/include
                           ${ROOT_INCLUDE_DIR})
target_link_libraries(${exe_name} PRIVATE ${ROOT_LIBRARIES})

install(TARGETS ${exe_name} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)