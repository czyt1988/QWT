# qwt
cmake_minimum_required(VERSION 3.5)

set(QWTPLOT3D_LIB_NAME "plot3d")
set(QWTPLOT3D_LIB_DESCRIPTION "QWT | 3D scientific plotting library")
set(QWTPLOT3D_LIB_VERSION "${QWT_VERSION}")

message("")
message("${QWT_LIB_NAME} ${QWT_VERSION} | ${QWTPLOT3D_LIB_DESCRIPTION}")

project(${QWTPLOT3D_LIB_NAME}
        VERSION ${QWTPLOT3D_LIB_VERSION}
        LANGUAGES CXX
        DESCRIPTION "${QWTPLOT3D_LIB_DESCRIPTION}"
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/moc)
########################################################
# files 
########################################################
set(QWTPLOT3D_HEADER_FILES)
set(QWTPLOT3D_SOURCE_FILES)


set(QWTPLOT3D_HEADER_PLOT_3D
    qwt3d_color.h
    qwt3d_global.h
    qwt3d_types.h
    qwt3d_axis.h
    qwt3d_coordsys.h
    qwt3d_drawable.h
    qwt3d_helper.h
    qwt3d_label.h
    qwt3d_colorlegend.h
    qwt3d_plot.h
    qwt3d_plotitem.h
    qwt3d_surface.h
    qwt3d_bar.h
    qwt3d_line3d.h
    qwt3d_enrichment.h
    qwt3d_enrichment_std.h
    qwt3d_autoscaler.h
    qwt3d_autoptr.h
    qwt3d_io.h
    qwt3d_io_reader.h
    qwt3d_scale.h
    qwt3d_portability.h
    qwt3d_mapping.h
    qwt3d_gridmapping.h
    qwt3d_parametricsurface.h
    qwt3d_function.h
    qwt3d_io_gl2ps.h
    qwt3d_colormap_color.h
    qwt3d_theme.h
    qwt3d_render_context.h
)
set(QWTPLOT3D_SOURCE_PLOT_3D
    qwt3d_axis.cpp
    qwt3d_color.cpp
    qwt3d_coordsys.cpp
    qwt3d_drawable.cpp
    qwt3d_mousekeyboard.cpp
    qwt3d_movements.cpp
    qwt3d_lighting.cpp
    qwt3d_colorlegend.cpp
    qwt3d_plot.cpp
    qwt3d_plotitem.cpp
    qwt3d_surface.cpp
    qwt3d_bar.cpp
    qwt3d_line3d.cpp
    qwt3d_label.cpp
    qwt3d_types.cpp
    qwt3d_enrichment_std.cpp
    qwt3d_autoscaler.cpp
    qwt3d_io_reader.cpp
    qwt3d_io.cpp
    qwt3d_scale.cpp
    qwt3d_gridmapping.cpp
    qwt3d_parametricsurface.cpp
    qwt3d_function.cpp
    qwt3d_io_gl2ps.cpp
    qwt3d_colormap_color.cpp
    qwt3d_theme.cpp
    qwt3d_render_context.cpp
    shaders/shaders.qrc
)
list(APPEND QWTPLOT3D_HEADER_FILES ${QWTPLOT3D_HEADER_PLOT_3D})
list(APPEND QWTPLOT3D_SOURCE_FILES ${QWTPLOT3D_SOURCE_PLOT_3D})


add_library(${QWTPLOT3D_LIB_NAME} SHARED
        ${QWTPLOT3D_HEADER_FILES}
        ${QWTPLOT3D_SOURCE_FILES}
)

add_library(${QWT_LIB_NAME}::${QWTPLOT3D_LIB_NAME} ALIAS ${QWTPLOT3D_LIB_NAME})


########################################################
# 依赖链接
########################################################
target_link_libraries(${QWTPLOT3D_LIB_NAME} PUBLIC
    ${QWT_LIB_NAME}::core
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)


find_package(OpenGL COMPONENTS OpenGL REQUIRED)
find_library(GL2PS_LIBRARY NAMES gl2ps)
if(GL2PS_LIBRARY)
    find_path (GL2PS_INCLUDE_DIR NAMES gl2ps.h REQUIRED)
    message(STATUS "Found GL2PS: ${GL2PS_LIBRARY}, with include: ${GL2PS_INCLUDE_DIR}")
endif()
target_link_libraries(${QWTPLOT3D_LIB_NAME} PUBLIC
    OpenGL::GLU
)

find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
    OpenGL
    REQUIRED
)
target_link_libraries(${QWTPLOT3D_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::OpenGL
)
if(${QT_VERSION_MAJOR} EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
        OpenGLWidgets
        REQUIRED
    )
    target_link_libraries(${QWTPLOT3D_LIB_NAME} PUBLIC
        Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    )
    message(STATUS "target link Qt${QT_VERSION_MAJOR}::OpenGLWidgets")
endif()


if(NOT GL2PS_LIBRARY)
    enable_language( C )
    target_sources(${QWTPLOT3D_LIB_NAME} PRIVATE "3rdparty/gl2ps/gl2ps.c" "3rdparty/gl2ps/gl2ps.h")
    target_include_directories(${QWTPLOT3D_LIB_NAME} PRIVATE "3rdparty/gl2ps" )
else()
    target_link_libraries(${QWTPLOT3D_LIB_NAME} ${GL2PS_LIBRARY})
    target_include_directories(${QWTPLOT3D_LIB_NAME} PRIVATE ${GL2PS_INCLUDE_DIR})
endif()

# gl2ps is always available (system or bundled) — enable vector export
# PUBLIC because qwt3d_plot.h exposes Qwt3DVectorWriter types in saveVector()
target_compile_definitions(${QWTPLOT3D_LIB_NAME} PUBLIC QWT3D_ENABLE_GL2PS)

########################################################
# target properties
########################################################
set_target_properties(${QWTPLOT3D_LIB_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${QWT_VERSION}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "${QWT_LIB_NAME}${QWTPLOT3D_LIB_NAME}"
)
########################################################
# 目标依赖目录
########################################################
target_include_directories(${QWTPLOT3D_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${QWT_LIB_NAME}> # version_file
    $<INSTALL_INTERFACE:include/${QWT_LIB_NAME}/${QWTPLOT3D_LIB_NAME}>
    $<INSTALL_INTERFACE:include/${QWT_LIB_NAME}/classincludes>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..># version_file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../classincludes>
)
target_compile_definitions(${QWTPLOT3D_LIB_NAME} PUBLIC QWT3D_DLL)
target_compile_definitions(${QWTPLOT3D_LIB_NAME} PRIVATE QWT3D_MAKEDLL)


########################################################
# install 
########################################################
include(GNUInstallDirs)
install(TARGETS ${QWTPLOT3D_LIB_NAME} # plot3d
    EXPORT ${QWT_TARGET_NAME}         # qwtTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    ${QWTPLOT3D_HEADER_FILES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${QWT_LIB_NAME}/${QWTPLOT3D_LIB_NAME}
    COMPONENT headers
)
########################################################
# dll资源信息
########################################################
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/create_win32_resource_version.cmake)
if(WIN32)
        create_win32_resource_version(
                TARGET ${QWTPLOT3D_LIB_NAME}
                FILENAME ${QWTPLOT3D_LIB_NAME}
                VERSION ${QWT_VERSION}
                EXT "dll"
                COMPANYNAME "QWT | http://qwt.sf.net"
                COPYRIGHT ""
                DESCRIPTION ${QWTPLOT3D_LIB_DESCRIPTION}
        )
endif()
