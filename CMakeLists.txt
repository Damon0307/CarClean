cmake_minimum_required(VERSION 2.8.4)

project(CarClean)

#include ($ENV{HOME}/configs/cross.cmake)

 option(ENABLE_CROSS_COMPILE "Enable cross compile" OFF)
 
 if(ENABLE_CROSS_COMPILE)
    include ($ENV{HOME}/configs/cross.cmake)
    set(CMAKE_BUILD_TYPE Relase)
endif ()
 
set(CMAKE_CXX_STANDARD 14)

# 将 ARM_FLAG 传递给配置文件
configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h @ONLY)


file(GLOB_RECURSE SOURCES  src/*.cpp src/*.c  )

include_directories(src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/spdlog)
 
message("SOURCES"  ${SOURCES})

add_executable(${CMAKE_PROJECT_NAME}    ${SOURCES})

target_link_libraries(${CMAKE_PROJECT_NAME}  -lpthread)
 
 