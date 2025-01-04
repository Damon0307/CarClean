# CrossCompileConfig.cmake

# 检查是否已经设置了CMAKE_SYSTEM_NAME，如果没有，则设置为Linux
if(NOT CMAKE_SYSTEM_NAME)
    set(CMAKE_SYSTEM_NAME Linux)
endif()

# 设置交叉编译工具链的前缀
set(CMAKE_C_COMPILER "arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "arm-rockchip830-linux-uclibcgnueabihf-g++")

# 设置sysroot路径 for ultra
#set(CMAKE_SYSROOT "/home/wjc/luckfox-pico/sysdrv/source/buildroot/buildroot-2023.02.6/output/host/arm-buildroot-linux-uclibcgnueabihf/sysroot")

#for pico promax
set(CMAKE_SYSROOT_PREFIX "/home/wjc/MyLuck/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/arm-rockchip830-linux-uclibcgnueabihf/sysroot")


# 设置CMake寻找库文件和头文件时的搜索路径
set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT}")

# 设置CMake在寻找程序时的搜索路径
set(CMAKE_PROGRAM_PATH "${CMAKE_SYSROOT}/usr/bin")

# 设置编译器搜索的起始路径
set(CMAKE_LIBRARY_PATH "${CMAKE_SYSROOT}/usr/lib")
set(CMAKE_INCLUDE_PATH "${CMAKE_SYSROOT}/usr/include")

# 禁止CMake自动寻找和设置系统头文件和库文件的路径
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)