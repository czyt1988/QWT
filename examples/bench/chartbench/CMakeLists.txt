cmake_minimum_required(VERSION 3.15)
project(chartbench LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# ---------------------------------------------------------------------------
# Qt
# ---------------------------------------------------------------------------
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.14 COMPONENTS Core Gui Widgets OpenGL PrintSupport REQUIRED)
if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 COMPONENTS OpenGLWidgets REQUIRED)
endif()

# Qt Charts is optional: without it the chartbench simply drops that adapter.
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Charts QUIET)

# ---------------------------------------------------------------------------
# Qwt (either from the parent build tree or from an installed package)
# ---------------------------------------------------------------------------
if(NOT TARGET qwt::plot)
    find_package(qwt REQUIRED)
endif()

# ---------------------------------------------------------------------------
# QIm - pulled in as a git submodule (https://gitee.com/czyt1988/qim).
# The adapter is skipped when the submodule has not been checked out.
# ---------------------------------------------------------------------------
set(CHARTBENCH_QIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qim")
if(EXISTS "${CHARTBENCH_QIM_DIR}/CMakeLists.txt")
    set(QIM_BUILD_EXAMPLES OFF CACHE BOOL "chartbench: QIm examples off" FORCE)
    set(QIM_ENABLE_BENCHMARK OFF CACHE BOOL "chartbench: QIm benchmark off" FORCE)
    set(QIM_BUILD_TESTS OFF CACHE BOOL "chartbench: QIm tests off" FORCE)
    add_subdirectory("${CHARTBENCH_QIM_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/qim")
    set(CHARTBENCH_WITH_QIM ON)
else()
    message(STATUS "chartbench: QIm submodule missing (${CHARTBENCH_QIM_DIR}) - QIm adapter disabled")
    set(CHARTBENCH_WITH_QIM OFF)
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(CHARTBENCH_SOURCES
    main.cpp
    BenchCommon.cpp
    PlotAdapter.cpp
    BenchSuites.cpp
    QwtAdapter.cpp
    QCustomPlotAdapter.cpp
    qcustomplot.cpp
)

if(TARGET Qt${QT_VERSION_MAJOR}::Charts)
    list(APPEND CHARTBENCH_SOURCES QtChartsAdapter.cpp)
    set(CHARTBENCH_HAVE_QTCHARTS ON)
else()
    message(STATUS "chartbench: Qt Charts not installed - QtCharts adapter disabled")
    set(CHARTBENCH_HAVE_QTCHARTS OFF)
endif()

if(CHARTBENCH_WITH_QIM)
    list(APPEND CHARTBENCH_SOURCES QImAdapter.cpp)
endif()

function(chartbench_add_target target)
    add_executable(${target} ${CHARTBENCH_SOURCES})
    target_compile_features(${target} PRIVATE cxx_std_17)

    target_link_libraries(${target} PRIVATE
        qwt::plot
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::OpenGL
        Qt${QT_VERSION_MAJOR}::PrintSupport
    )
    if(QT_VERSION_MAJOR EQUAL 6)
        target_link_libraries(${target} PRIVATE Qt6::OpenGLWidgets)
    endif()
    if(CHARTBENCH_HAVE_QTCHARTS)
        target_compile_definitions(${target} PRIVATE CHARTBENCH_HAVE_QTCHARTS=1)
        target_link_libraries(${target} PRIVATE Qt${QT_VERSION_MAJOR}::Charts)
    endif()
    if(CHARTBENCH_WITH_QIM)
        target_compile_definitions(${target} PRIVATE CHARTBENCH_HAVE_QIM=1)
        target_link_libraries(${target} PRIVATE QIm::Core QIm::Widgets)
    endif()

    set_target_properties(${target} PROPERTIES
        AUTOMOC ON
        CXX_EXTENSIONS OFF
        DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endfunction()

# Raster build of QCustomPlot (the default).
chartbench_add_target(chartbench)

# Second build of the very same sources with QCustomPlot switched to its
# OpenGL paint buffer. QCUSTOMPLOT_USE_OPENGL is a compile time switch, so the
# two backends cannot live in one binary.
chartbench_add_target(chartbench_qcpgl)
target_compile_definitions(chartbench_qcpgl PRIVATE QCUSTOMPLOT_USE_OPENGL=1)
if(WIN32)
    # QCPPaintBufferGlFbo calls glClear/glClearColor directly.
    target_link_libraries(chartbench_qcpgl PRIVATE opengl32)
endif()
