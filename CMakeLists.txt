
# QWT CMake By Czyt1988

cmake_minimum_required(VERSION 3.5)

########################################################
# Version Definition
########################################################
set(QWT_VERSION_MAJOR 7)
set(QWT_VERSION_MINOR 4)
set(QWT_VERSION_PATCH 0)
set(QWT_VERSION "${QWT_VERSION_MAJOR}.${QWT_VERSION_MINOR}.${QWT_VERSION_PATCH}")
message(STATUS "QWT Version is ${QWT_VERSION}")


########################################################
# Project Definition
########################################################
set(QWT_PROJECT_NAME "qwt-project")
set(QWT_LIB_NAME "qwt")
set(QWT_TARGET_NAME "qwtTargets") # This is the unified export set for all modules.
project(${QWT_PROJECT_NAME} 
        VERSION ${QWT_VERSION} 
        LANGUAGES CXX
        DESCRIPTION "Qwt - Qt Widgets for Technical Applications"
)

########################################################
# General Settings
########################################################
set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

# Detect Qt version first (needed for C++ standard selection)
set(QWT_MIN_QT_VERSION 5.12)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

# C++ standard: Qt6 requires C++17, Qt5 uses C++11
# Code is written to be C++11 compatible (qwt_make_unique, qwt_as_const, etc.)
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 11)
endif()
# Enforce the requirement
set(CMAKE_CXX_STANDARD_REQUIRED ON)

########################################################
# QWT Configuration
########################################################
option(QWT_CONFIG_QWTPLOT "Enable all classes needed to use QwtPlot" ON)
option(QWT_CONFIG_QWTPOLAR "Enable all classes needed for QwtPolar functionality. Requires QWT_CONFIG_QWTPLOT to be enabled" ON)
option(QWT_CONFIG_QWTWIDGETS "Enable all other widgets (sliders, dials, etc.) besides QwtPlot" ON)
option(QWT_CONFIG_QWTSVG "Enable SVG support for plot canvas and SVG export functionality" ON)
option(QWT_CONFIG_QWTOPENGL "Enable OpenGL plot canvas support" ON)
option(QWT_CONFIG_QWTPLOT_3D "Enable QwtPlot3D module" ON)

option(QWT_CONFIG_BUILD_EXAMPLE "Build example applications" ON)
option(QWT_CONFIG_BUILD_PLAYGROUND "Build playground applications" ON)
option(QWT_CONFIG_BUILD_STATIC_EXAMPLE "Build static examples (takes relatively long compilation time)" ON)
option(QWT_CONFIG_BUILD_TESTS "Build test applications" OFF)

########################################################
# MSVC Settings
########################################################
if(MSVC)
    # Enable UTF-8 encoding for MSVC
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    # Ensure __cplusplus is defined when using MSVC and enable parallel build
    string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus /MP")
endif()

if(MINGW)
    # GCC defaults -finput-charset=UTF-8 but -fexec-charset follows the
    # system code page (e.g. GBK/CP936 on Chinese Windows), which corrupts
    # UTF-8 narrow string literals fed to QStringLiteral / QString::fromUtf8.
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

########################################################
# Install Path Settings
########################################################

# Determine platform architecture
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(QWT_PLATFORM_NAME "x86")
else()
    set(QWT_PLATFORM_NAME "x64")
endif()

set(QWT_BIN_DIR_NAME bin_qt${QT_VERSION}_${CMAKE_BUILD_TYPE}_${QWT_PLATFORM_NAME})


########################################################
# Source Directories
########################################################
add_subdirectory(src)

if(QWT_CONFIG_BUILD_EXAMPLE)
    add_subdirectory(examples)
endif()
if(QWT_CONFIG_BUILD_PLAYGROUND)
    add_subdirectory(playground)
endif()
if(QWT_CONFIG_BUILD_TESTS)
    add_subdirectory(tests)
endif()

########################################################
# Python Bindings (PySide6 / Shiboken6)
########################################################
option(QWT_CONFIG_BUILD_BINDINGS "Build PySide6 Python bindings via Shiboken6" OFF)
if(QWT_CONFIG_BUILD_BINDINGS)
    add_subdirectory(bindings)
endif()

########################################################
# install
########################################################
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${QWT_LIB_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qwtConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${QWT_LIB_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${QWT_LIB_NAME}
)
#Unified export of all module targets
install(EXPORT ${QWT_TARGET_NAME}
    FILE ${QWT_TARGET_NAME}.cmake
    NAMESPACE ${QWT_LIB_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${QWT_LIB_NAME}
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${QWT_LIB_NAME}ConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${QWT_LIB_NAME}Config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${QWT_LIB_NAME}
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/classincludes"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${QWT_LIB_NAME}
)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qwt_version_info.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qwt_global.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/qwtcore_global.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${QWT_LIB_NAME}
    COMPONENT headers
)
