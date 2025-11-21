
# QWT CMake By Czyt1988

cmake_minimum_required(VERSION 3.5)

########################################################
# 版本定义
########################################################
set(QWT_VERSION_MAJOR 7)
set(QWT_VERSION_MINOR 0)
set(QWT_VERSION_PATCH 7)
set(QWT_VERSION "${QWT_VERSION_MAJOR}.${QWT_VERSION_MINOR}.${QWT_VERSION_PATCH}")
message(STATUS "QWT Version is ${QWT_VERSION}")


########################################################
# 定义工程
########################################################
set(QWT_PROJECT_NAME "qwt-project")

project(${QWT_PROJECT_NAME} 
        VERSION ${QWT_VERSION} 
        LANGUAGES CXX
        DESCRIPTION "Qwt - Qt Widgets for Technical Applications"
        )

########################################################
# 一些常规设置
########################################################


set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

# c++标准，要求为17
set(CMAKE_CXX_STANDARD 14)
# 强制要求
set(CMAKE_CXX_STANDARD_REQUIRED ON)

########################################################
# QWT设置
########################################################
option(QWT_CONFIG_QWTPLOT "QwtPlot enables all classes, that are needed to use the QwtPlot" ON)
option(QWT_CONFIG_QWTPOLAR "QwtPolar enables all classes, that are needed to use the QwtPolar classes. It can't be enabled without also enabling QwtPlot" ON)
option(QWT_CONFIG_QWTWIDGETS "QwtWidgets enables all classes, that are needed to use the all other widgets (sliders, dials, ...), beside QwtPlot." ON)
option(QWT_CONFIG_QWTSVG "If you want to display svg images on the plot canvas, or export a plot to a SVG document" ON)
option(QWT_CONFIG_QWTOPENGL "If you want to use a OpenGL plot canvas" ON)
option(QWT_CONFIG_BUILD_EXAMPLE "build examples" ON)
option(QWT_CONFIG_BUILD_PLAYGROUND "build playground" ON)
option(QWT_CONFIG_BUILD_STATIC_EXAMPLE "build static example,Building the static examples will take a relatively long compilation time." OFF)
########################################################
# MSVC设置
########################################################
if(MSVC)
# msvc utf-8
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    # make sure __cplusplus is defined when using msvc and enable parallel build
    string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus /MP")
endif()

########################################################
# 安装路径设置
########################################################
set(QWT_MIN_QT_VERSION 5.10)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(QWT_PLATFORM_NAME "x86")
else()
    set(QWT_PLATFORM_NAME "x64")
endif()
set(QWT_BIN_DIR_NAME bin_qt${QT_VERSION}_${CMAKE_BUILD_TYPE}_${DA_PLATFORM_NAME})


##################################
# install
##################################
message(STATUS "CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")

########################################################
# 源码路径
########################################################
add_subdirectory(src)

if(QWT_CONFIG_BUILD_EXAMPLE)
    add_subdirectory(examples)
endif()
if(QWT_CONFIG_BUILD_PLAYGROUND)
    add_subdirectory(playground)
endif()

