# qwt core
cmake_minimum_required(VERSION 3.10)

set(QWTCORE_LIB_NAME "core")
set(QWTCORE_LIB_DESCRIPTION "QWT | Core color and palette utilities")

message("")
message("${QWT_LIB_NAME} ${QWT_VERSION} | ${QWTCORE_LIB_DESCRIPTION}")

project(${QWTCORE_LIB_NAME}
        VERSION ${QWT_VERSION}
        LANGUAGES CXX
        DESCRIPTION "${QWTCORE_LIB_DESCRIPTION}"
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

########################################################
# files
########################################################
set(QWTCORE_HEADER_FILES
    qwtcore_global.h
    qwt_colormap.h
    qwt_color_cycle.h
    qwt_colormap_preset.h
    qwt_math.h
    qwt_interval.h
    qwt_point_3d.h
    qwt_point_polar.h
    qwt_system_clock.h
    qwt_algorithm.hpp
    qwt_qt5qt6_compat.hpp
    qwt_samples.h
    qwt_bezier.h
    qwt_clipper.h
    qwt_date.h
    qwt_transform.h
    qwt_scale_map.h
    qwt_scale_div.h
    qwt_simd_argminmax.h
    qwt_box_statistics.h
    qwt_grid_data.hpp
    qwt_scale_engine.h
    qwt_series_data.h
    qwt_point_data.h
    qwt_series_store.h
    qwt_raster_data.h
    qwt_matrix_raster_data.h
    qwt_grid_raster_data.h
)

set(QWTCORE_SOURCE_FILES
    qwt_colormap.cpp
    qwt_color_cycle.cpp
    qwt_colormap_preset.cpp
    qwt_math.cpp
    qwt_interval.cpp
    qwt_point_3d.cpp
    qwt_point_polar.cpp
    qwt_system_clock.cpp
    qwt_bezier.cpp
    qwt_clipper.cpp
    qwt_date.cpp
    qwt_transform.cpp
    qwt_scale_map.cpp
    qwt_scale_div.cpp
    qwt_simd_argminmax.cpp
    qwt_box_statistics.cpp
    qwt_scale_engine.cpp
    qwt_series_data.cpp
    qwt_point_data.cpp
    qwt_raster_data.cpp
    qwt_matrix_raster_data.cpp
    qwt_grid_raster_data.cpp
)

########################################################
# library target
########################################################
add_library(${QWTCORE_LIB_NAME} SHARED
    ${QWTCORE_HEADER_FILES}
    ${QWTCORE_SOURCE_FILES}
)

add_library(${QWT_LIB_NAME}::${QWTCORE_LIB_NAME} ALIAS ${QWTCORE_LIB_NAME})

########################################################
# dependencies
########################################################
target_link_libraries(${QWTCORE_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
)

########################################################
# target properties
########################################################
set_target_properties(${QWTCORE_LIB_NAME} PROPERTIES
    AUTOMOC ON
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${QWT_VERSION}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "${QWT_LIB_NAME}${QWTCORE_LIB_NAME}"
)

target_include_directories(${QWTCORE_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${QWT_LIB_NAME}> # version_file
    $<INSTALL_INTERFACE:include/${QWT_LIB_NAME}/${QWTCORE_LIB_NAME}>
    $<INSTALL_INTERFACE:include/${QWT_LIB_NAME}/classincludes>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> # version_file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../classincludes>
)

target_compile_definitions(${QWTCORE_LIB_NAME} PUBLIC QWTCORE_DLL)
target_compile_definitions(${QWTCORE_LIB_NAME} PRIVATE QWTCORE_MAKEDLL)

########################################################
# install
########################################################
include(GNUInstallDirs)
install(TARGETS ${QWTCORE_LIB_NAME} # core
    EXPORT ${QWT_TARGET_NAME}       # qwtTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    ${QWTCORE_HEADER_FILES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${QWT_LIB_NAME}/${QWTCORE_LIB_NAME}
    COMPONENT headers
)

########################################################
# dll resource info
########################################################
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/create_win32_resource_version.cmake)
if(WIN32)
    create_win32_resource_version(
        TARGET ${QWTCORE_LIB_NAME}
        FILENAME ${QWTCORE_LIB_NAME}
        VERSION ${QWT_VERSION}
        EXT "dll"
        COMPANYNAME "QWT | http://qwt.sf.net"
        COPYRIGHT ""
        DESCRIPTION ${QWTCORE_LIB_DESCRIPTION}
    )
endif()
