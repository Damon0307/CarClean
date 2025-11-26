cmake_minimum_required(VERSION 3.1.0)

project(CarClean)
 
set(ARM_FLAG 1) # 这里只是一个例子，该变量可能来自其他地方

if (ARM_FLAG)
   include (luck_cross.cmake)
    set(CMAKE_BUILD_TYPE Debug)
    #set(CMAKE_BUILD_TYPE Release)
    #include (/opt/luck_toolchain/arm-rockchip830-linux-uclibcgnueabihf/arm-toolchain.cmake)
else ()
    set(CMAKE_BUILD_TYPE Debug)
endif ()
 
set(CMAKE_CXX_STANDARD 14)


# -------------------------- 调试信息关键配置（新增/修改）--------------------------
# 1. 强制保留调试符号（-g），禁用优化（-O0）—— 避免优化导致栈信息错乱
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0") # 若有C代码，需添加C编译选项

# 2. 禁止Debug版本剥离符号（交叉编译环境易出现符号被自动strip的问题）
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-Wl,-no-strip" CACHE INTERNAL "")

# 3. 确保Release模式不影响（可选，防止误操作）
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
# --------------------------------------------------------------------------------

# 将 ARM_FLAG 传递给配置文件
configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h @ONLY)

file(GLOB_RECURSE SOURCES  src/*.cpp src/*.c  )

# 合并重复的include路径（优化，不影响功能）
include_directories(
    src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spdlog
)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

message("SOURCES: " ${SOURCES}) # 补充冒号，日志更清晰

add_executable(${CMAKE_PROJECT_NAME}    ${SOURCES})

target_link_libraries(${CMAKE_PROJECT_NAME}  -lpthread)
#IO库
#target_link_libraries(${CMAKE_PROJECT_NAME}  gpiod)

#决定使用sys
 

 