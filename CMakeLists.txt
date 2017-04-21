cmake_minimum_required(VERSION 3.6)
project(GDALwork)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp sampling.cpp sampling.h ppm.cpp ppm.h)
add_executable(GDALwork ${SOURCE_FILES})
find_package(GDAL REQUIRED)
if (GDAL_FOUND)
    include_directories(${GDAL_INCLUDE_DIRS})
    target_link_libraries(GDALwork ${GDAL_LIBRARIES})
endif(GDAL_FOUND)