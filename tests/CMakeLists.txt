# QWT Tests CMake Configuration

cmake_minimum_required(VERSION 3.10)
project(QwtTests LANGUAGES CXX)

# Require Qt Test module
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.8 COMPONENTS Core Gui Widgets Test REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Add test subdirectories
add_subdirectory(axis_corner_test)
add_subdirectory(box_statistics_test)
add_subdirectory(downsampling_test)
add_subdirectory(picker_click_test)
add_subdirectory(picker_group_click_test)
add_subdirectory(clipper_test)
add_subdirectory(color_cycle_test)
add_subdirectory(flat_style_test)
add_subdirectory(wheel_scrollarea_test)
add_subdirectory(nan_curve_test)

# The 3D tests require the OpenGLWidgets CMake component, that
# exists as a separate package since Qt 6 only
if(QT_VERSION_MAJOR EQUAL 6)
    add_subdirectory(waterfall_test)
    add_subdirectory(plot3d_io_test)
    add_subdirectory(plot3d_rotation_test)
endif()