# QWT Tests CMake Configuration

cmake_minimum_required(VERSION 3.5)
project(QwtTests LANGUAGES CXX)

# Require Qt Test module
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.8 COMPONENTS Core Gui Widgets Test REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Add test subdirectories
add_subdirectory(picker_click_test)
add_subdirectory(picker_group_click_test)
add_subdirectory(color_cycle_test)
add_subdirectory(flat_style_test)
add_subdirectory(wheel_scrollarea_test)