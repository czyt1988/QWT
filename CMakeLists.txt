
# QWT CMake By Czyt1988

cmake_minimum_required(VERSION 3.5)

########################################################
# 版本定义
########################################################
set(QWT_VERSION_MAJOR 6)
set(QWT_VERSION_MINOR 2)
set(QWT_VERSION_PATCH 0)
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
set(CMAKE_RELWITHDEBINFO_POSTFIX "rd" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX "s" CACHE STRING "add a postfix, usually empty on windows")

# c++标准，要求为17
set(CMAKE_CXX_STANDARD 17)
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
set(DA_MIN_QT_VERSION 5.14)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(DA_PLATFORM_NAME "x86")
else()
    set(DA_PLATFORM_NAME "x64")
endif()
set(DA_BIN_DIR_NAME bin_qt${QT_VERSION}_${CMAKE_BUILD_TYPE}_${DA_PLATFORM_NAME})


##################################
# install
##################################
message(STATUS "CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")

########################################################
# 源码路径
########################################################
add_subdirectory(src)



