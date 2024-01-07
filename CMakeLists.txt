cmake_minimum_required(VERSION 2.8.4)

project(CarClean)
#include ($ENV{HOME}/configs/cross.cmake)
 
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE "Debug")

file(GLOB_RECURSE SOURCES  src/*.cpp src/*.c  ) 

include_directories(src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/spdlog)

message("SOURCES"  ${SOURCES})

add_executable(${CMAKE_PROJECT_NAME}    ${SOURCES})

target_link_libraries(${CMAKE_PROJECT_NAME}  -lpthread)
