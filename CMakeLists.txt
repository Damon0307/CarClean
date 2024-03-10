cmake_minimum_required(VERSION 2.8.4)

project(CarClean)
 
set(ARM_FLAG ON) # 这里只是一个例子，该变量可能来自其他地方

if(${ARM_FLAG})
    include ($ENV{HOME}/configs/cross.cmake)
    set(POCO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/armpoco)
else()
find_package(Poco REQUIRED COMPONENTS Net Foundation Util XML JSON)
     message("Poco include dirs: " ${Poco_INCLUDE_DIRS})
     message("Poco libraries: " ${Poco_LIBRARIES})
     set(POCO_DIR ${Poco_INCLUDE_DIRS}) # 将Poco组件的头文件目录设置为POCO_DIR  
     set(CMAKE_BUILD_TYPE Debug)
endif()
 
message("POCO_DIR ->" ${POCO_DIR})
 

set(CMAKE_CXX_STANDARD 14)

file(GLOB_RECURSE SOURCES  src/*.cpp src/*.c  )

include_directories(src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/spdlog)
include_directories(${POCO_DIR}/include)
link_directories(${POCO_DIR}/lib)


message("SOURCES"  ${SOURCES})

add_executable(${CMAKE_PROJECT_NAME}    ${SOURCES})

target_link_libraries(${CMAKE_PROJECT_NAME}  -lpthread)
 
# target_link_libraries(${CMAKE_PROJECT_NAME}  ${POCO_DIR}/lib/libPocoNet.so)
# target_link_libraries(${CMAKE_PROJECT_NAME}  ${POCO_DIR}/lib/libPocoFoundation.so)
# target_link_libraries(${CMAKE_PROJECT_NAME}  ${POCO_DIR}/lib/libPocoUtil.so)
# target_link_libraries(${CMAKE_PROJECT_NAME}  ${POCO_DIR}/lib/libPocoXML.so)
# target_link_libraries(${CMAKE_PROJECT_NAME}  ${POCO_DIR}/lib/libPocoJSON.so)

if (${ARM_FLAG})
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        ${POCO_DIR}/lib/libPocoNet.so
        ${POCO_DIR}/lib/libPocoFoundation.so
        ${POCO_DIR}/lib/libPocoUtil.so
        ${POCO_DIR}/lib/libPocoXML.so
        ${POCO_DIR}/lib/libPocoJSON.so
    )
else()
    # 使用find_package找到的Poco组件链接库
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Poco::Net Poco::Foundation Poco::Util Poco::XML Poco::JSON)
endif()
