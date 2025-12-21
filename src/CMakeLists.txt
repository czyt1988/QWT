
# Qwt库的cmake编译

cmake_minimum_required(VERSION 3.5)

set(QWT_LIB_NAME "qwt")
set(QWT_LIB_DESCRIPTION "QWT | Qt Widgets for Technical Applications")
set(QWT_LIB_VERSION "${QWT_VERSION}")

message("")
message("${QWT_LIB_NAME}${QWT_LIB_VERSION} | ${QWT_LIB_DESCRIPTION}")

project(${QWT_LIB_NAME} 
        VERSION ${QWT_LIB_VERSION} 
        LANGUAGES CXX
        DESCRIPTION "DAWorkbench | ${QWT_LIB_DESCRIPTION}"
)

########################################################
# 通用常规设置
########################################################
# C++标准要求最低C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 编译选项
set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
########################################################
# MSVC设置
########################################################
if(MSVC)
# msvc utf-8
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()
########################################################
# Qt
########################################################
# Qt 5.12 或更高版本有完善的 C++17 支持
set(QWT_MIN_QT_VERSION 5.12)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
    Core
    Gui
    Widgets
    Concurrent
    PrintSupport
    REQUIRED
)

if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

########################################################
# 目录包含
########################################################
# 包含自身目录
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/moc)
########################################################
# 文件加载
########################################################
set(QWT_HEADER_BASE
        qwt_version_info.h
        qwt_abstract_scale_draw.h
        qwt_bezier.h
        qwt_clipper.h
        qwt_color_map.h
        qwt_column_symbol.h
        qwt_date.h
        qwt_date_scale_draw.h
        qwt_date_scale_engine.h
        qwt_dyngrid_layout.h
        qwt_global.h
        qwt_graphic.h
        qwt_interval.h
        qwt_interval_symbol.h
        qwt_math.h
        qwt_algorithm.hpp
        qwt_qt5qt6_compat.hpp
        qwt_magnifier.h
        qwt_null_paintdevice.h
        qwt_painter.h
        qwt_painter_command.h
        qwt_cache_panner.h
        qwt_picker.h
        qwt_picker_machine.h
        qwt_pixel_matrix.h
        qwt_point_3d.h
        qwt_point_polar.h
        qwt_round_scale_draw.h
        qwt_scale_div.h
        qwt_scale_draw.h
        qwt_scale_engine.h
        qwt_scale_map.h
        qwt_spline.h
        qwt_spline_basis.h
        qwt_spline_parametrization.h
        qwt_spline_local.h
        qwt_spline_cubic.h
        qwt_spline_pleasing.h
        qwt_spline_polynomial.h
        qwt_symbol.h
        qwt_system_clock.h
        qwt_stylesheet_recorder.h
        qwt_text_engine.h
        qwt_text_label.h
        qwt_text.h
        qwt_transform.h
        qwt_widget_overlay.h
        qwt_utils.h
)
set(QWT_SOURCE_BASE
        qwt_abstract_scale_draw.cpp
        qwt_bezier.cpp
        qwt_clipper.cpp
        qwt_color_map.cpp
        qwt_column_symbol.cpp
        qwt_date.cpp
        qwt_date_scale_draw.cpp
        qwt_date_scale_engine.cpp
        qwt_dyngrid_layout.cpp
        qwt_event_pattern.cpp
        qwt_graphic.cpp
        qwt_interval.cpp
        qwt_interval_symbol.cpp
        qwt_math.cpp
        qwt_magnifier.cpp
        qwt_null_paintdevice.cpp
        qwt_painter.cpp
        qwt_painter_command.cpp
        qwt_cache_panner.cpp
        qwt_picker.cpp
        qwt_picker_machine.cpp
        qwt_pixel_matrix.cpp
        qwt_point_3d.cpp
        qwt_point_polar.cpp
        qwt_round_scale_draw.cpp
        qwt_scale_div.cpp
        qwt_scale_draw.cpp
        qwt_scale_map.cpp
        qwt_scale_engine.cpp
        qwt_spline.cpp
        qwt_spline_basis.cpp
        qwt_spline_parametrization.cpp
        qwt_spline_local.cpp
        qwt_spline_cubic.cpp
        qwt_spline_pleasing.cpp
        qwt_spline_polynomial.cpp
        qwt_symbol.cpp
        qwt_system_clock.cpp
        qwt_stylesheet_recorder.cpp
        qwt_text_engine.cpp
        qwt_text_label.cpp
        qwt_text.cpp
        qwt_transform.cpp
        qwt_widget_overlay.cpp
        qwt_utils.cpp
)

set(QWT_HEADER_FILES ${QWT_HEADER_BASE})
set(QWT_SOURCE_FILES ${QWT_SOURCE_BASE})

if(QWT_CONFIG_QWTPLOT)
    set(QWT_HEADER_PLOT
        qwt_axis.h
        qwt_axis_id.h
        qwt_curve_fitter.h
        qwt_spline_curve_fitter.h
        qwt_weeding_curve_fitter.h
        qwt_event_pattern.h
        qwt_abstract_legend.h
        qwt_legend.h
        qwt_legend_data.h
        qwt_legend_label.h
        qwt_plot.h
        qwt_plot_renderer.h
        qwt_plot_curve.h
        qwt_plot_dict.h
        qwt_plot_directpainter.h
        qwt_plot_graphicitem.h
        qwt_plot_grid.h
        qwt_plot_histogram.h
        qwt_plot_item.h
        qwt_plot_abstract_barchart.h
        qwt_plot_barchart.h
        qwt_plot_multi_barchart.h
        qwt_plot_intervalcurve.h
        qwt_plot_tradingcurve.h
        qwt_plot_layout_engine.h
        qwt_plot_layout.h
        qwt_parasite_plot_layout.h
        qwt_plot_scale_event_dispatcher.h
        qwt_plot_marker.h
        qwt_plot_zoneitem.h
        qwt_plot_textlabel.h
        qwt_plot_rasteritem.h
        qwt_plot_spectrogram.h
        qwt_plot_spectrocurve.h
        qwt_plot_scaleitem.h
        qwt_plot_legenditem.h
        qwt_plot_seriesitem.h
        qwt_plot_shapeitem.h
        qwt_plot_vectorfield.h
        qwt_plot_abstract_canvas.h
        qwt_plot_canvas.h
        qwt_plot_transparent_canvas.h
        qwt_plot_panner.h
        qwt_plot_cache_panner.h
        qwt_canvas_picker.h
        qwt_plot_picker.h
        qwt_plot_series_data_picker.h
        qwt_plot_axis_zoomer.h
        qwt_plot_canvas_zoomer.h
        qwt_plot_magnifier.h
        qwt_plot_rescaler.h
        qwt_point_mapper.h
        qwt_grid_data.hpp
        qwt_raster_data.h
        qwt_matrix_raster_data.h
        qwt_grid_raster_data.h
        qwt_vectorfield_symbol.h
        qwt_sampling_thread.h
        qwt_samples.h
        qwt_series_data.h
        qwt_series_store.h
        qwt_point_data.h
        qwt_scale_widget.h
        qwt_figure_layout.h
        qwt_figure.h
        qwt_figure_widget_overlay.h
    )
    set(QWT_SOURCE_PLOT
        qwt_curve_fitter.cpp
        qwt_spline_curve_fitter.cpp
        qwt_weeding_curve_fitter.cpp
        qwt_abstract_legend.cpp
        qwt_legend.cpp
        qwt_legend_data.cpp
        qwt_legend_label.cpp
        qwt_plot.cpp
        qwt_plot_renderer.cpp
        qwt_plot_axis.cpp
        qwt_plot_curve.cpp
        qwt_plot_dict.cpp
        qwt_plot_directpainter.cpp
        qwt_plot_graphicitem.cpp
        qwt_plot_grid.cpp
        qwt_plot_histogram.cpp
        qwt_plot_item.cpp
        qwt_plot_abstract_barchart.cpp
        qwt_plot_barchart.cpp
        qwt_plot_multi_barchart.cpp
        qwt_plot_intervalcurve.cpp
        qwt_plot_zoneitem.cpp
        qwt_plot_tradingcurve.cpp
        qwt_plot_spectrogram.cpp
        qwt_plot_spectrocurve.cpp
        qwt_plot_scaleitem.cpp
        qwt_plot_legenditem.cpp
        qwt_plot_seriesitem.cpp
        qwt_plot_shapeitem.cpp
        qwt_plot_vectorfield.cpp
        qwt_plot_marker.cpp
        qwt_plot_textlabel.cpp
        qwt_plot_layout_engine.cpp
        qwt_plot_layout.cpp
        qwt_parasite_plot_layout.cpp
        qwt_plot_scale_event_dispatcher.cpp
        qwt_plot_abstract_canvas.cpp
        qwt_plot_canvas.cpp
        qwt_plot_transparent_canvas.cpp
        qwt_plot_panner.cpp
        qwt_plot_cache_panner.cpp
        qwt_plot_rasteritem.cpp
        qwt_canvas_picker.cpp
        qwt_plot_picker.cpp
        qwt_plot_series_data_picker.cpp
        qwt_plot_axis_zoomer.cpp
        qwt_plot_canvas_zoomer.cpp
        qwt_plot_magnifier.cpp
        qwt_plot_rescaler.cpp
        qwt_point_mapper.cpp
        qwt_raster_data.cpp
        qwt_matrix_raster_data.cpp
        qwt_grid_raster_data.cpp
        qwt_vectorfield_symbol.cpp
        qwt_sampling_thread.cpp
        qwt_series_data.cpp
        qwt_point_data.cpp
        qwt_scale_widget.cpp
        qwt_figure_layout.cpp
        qwt_figure.cpp
        qwt_figure_widget_overlay.cpp
    )

    list(APPEND QWT_HEADER_FILES ${QWT_HEADER_PLOT})
    list(APPEND QWT_SOURCE_FILES ${QWT_SOURCE_PLOT})
endif()

if(QWT_CONFIG_QWTOPENGL)
    set(QWT_HEADER_OPENGL
        qwt_plot_opengl_canvas.h
    )
    set(QWT_SOURCE_OPENGL
        qwt_plot_opengl_canvas.cpp
    )
    if(${QT_VERSION_MAJOR} LESS 6)
        list(APPEND QWT_HEADER_OPENGL qwt_plot_glcanvas.h)
        list(APPEND QWT_SOURCE_OPENGL qwt_plot_glcanvas.cpp)
    endif()
    list(APPEND QWT_HEADER_FILES ${QWT_HEADER_OPENGL})
    list(APPEND QWT_SOURCE_FILES ${QWT_SOURCE_OPENGL})
endif()

if(QWT_CONFIG_QWTSVG)
    set(QWT_HEADER_SVG
        qwt_plot_svgitem.h
    )
    set(QWT_SOURCE_SVG
        qwt_plot_svgitem.cpp
    )
    list(APPEND QWT_HEADER_FILES ${QWT_HEADER_SVG})
    list(APPEND QWT_SOURCE_FILES ${QWT_SOURCE_SVG})
endif()

if(QWT_CONFIG_QWTPOLAR)
    set(QWT_HEADER_POLAR
        qwt_polar.h
        qwt_polar_canvas.h
        qwt_polar_curve.h
        qwt_polar_fitter.h
        qwt_polar_grid.h
        qwt_polar_itemdict.h
        qwt_polar_item.h
        qwt_polar_layout.h
        qwt_polar_magnifier.h
        qwt_polar_marker.h
        qwt_polar_cache_panner.h
        qwt_polar_picker.h
        qwt_polar_plot.h
        qwt_polar_renderer.h
        qwt_polar_spectrogram.h
    )
    set(QWT_SOURCE_POLAR
        qwt_polar_canvas.cpp
        qwt_polar_curve.cpp
        qwt_polar_fitter.cpp
        qwt_polar_grid.cpp
        qwt_polar_item.cpp
        qwt_polar_itemdict.cpp
        qwt_polar_layout.cpp
        qwt_polar_magnifier.cpp
        qwt_polar_marker.cpp
        qwt_polar_cache_panner.cpp
        qwt_polar_picker.cpp
        qwt_polar_plot.cpp
        qwt_polar_renderer.cpp
        qwt_polar_spectrogram.cpp
    )
    list(APPEND QWT_HEADER_FILES ${QWT_HEADER_POLAR})
    list(APPEND QWT_SOURCE_FILES ${QWT_SOURCE_POLAR})
endif()


if(QWT_CONFIG_QWTWIDGETS)
    set(QWT_HEADER_WIDGETS
        qwt_abstract_slider.h
        qwt_abstract_scale.h
        qwt_arrow_button.h
        qwt_analog_clock.h
        qwt_compass.h
        qwt_compass_rose.h
        qwt_counter.h
        qwt_dial.h
        qwt_dial_needle.h
        qwt_knob.h
        qwt_slider.h
        qwt_thermo.h
        qwt_wheel.h
    )
    set(QWT_SOURCE_WIDGETS
        qwt_abstract_slider.cpp
        qwt_abstract_scale.cpp
        qwt_arrow_button.cpp
        qwt_analog_clock.cpp
        qwt_compass.cpp
        qwt_compass_rose.cpp
        qwt_counter.cpp
        qwt_dial.cpp
        qwt_dial_needle.cpp
        qwt_knob.cpp
        qwt_slider.cpp
        qwt_thermo.cpp
        qwt_wheel.cpp
    )
    list(APPEND QWT_HEADER_FILES ${QWT_HEADER_WIDGETS})
    list(APPEND QWT_SOURCE_FILES ${QWT_SOURCE_WIDGETS})
endif()

#QT5_WRAP_CPP(QWT_HEADERS_MOC ${QWT_HEADER_FILES})
add_library(${QWT_LIB_NAME} SHARED
        ${QWT_HEADER_FILES}
        ${QWT_SOURCE_FILES}
#        ${QWT_HEADERS_MOC}
)
# 启用别名，可以通过qwt::qwt引入
add_library(${QWT_LIB_NAME}::${QWT_LIB_NAME} ALIAS ${QWT_LIB_NAME})

# 构建库定义的宏
target_compile_definitions(${QWT_LIB_NAME} PRIVATE QT_DLL)
# 此宏必须为public
target_compile_definitions(${QWT_LIB_NAME} PUBLIC QWT_DLL)
#此宏必须为private
target_compile_definitions(${QWT_LIB_NAME} PRIVATE QWT_MAKEDLL)
########################################################
# 依赖链接
########################################################
target_link_libraries(${QWT_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

if(QWT_CONFIG_QWTOPENGL)
    find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
        OpenGL
        REQUIRED
    )
    target_link_libraries(${QWT_LIB_NAME} PRIVATE
        Qt${QT_VERSION_MAJOR}::OpenGL
    )
    if(${QT_VERSION_MAJOR} EQUAL 6)
        find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
            OpenGLWidgets
            REQUIRED
        )
        target_link_libraries(${QWT_LIB_NAME} PRIVATE
            Qt${QT_VERSION_MAJOR}::OpenGLWidgets
        )
    endif()
endif()

if(QWT_CONFIG_QWTSVG)
    find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
        Svg
        REQUIRED
    )
    target_link_libraries(${QWT_LIB_NAME} PRIVATE
        Qt${QT_VERSION_MAJOR}::Svg
    )
endif()
########################################################
# 设置target属性，以及Qt的moc
########################################################
# 在同一个 CMake build tree 中，每个项目的 targets 对外都是可见的
set_target_properties(${QWT_LIB_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${QWT_LIB_VERSION}
    EXPORT_NAME ${QWT_LIB_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
########################################################
# 目标依赖目录
########################################################
target_include_directories(${QWT_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${QWT_LIB_NAME}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../classincludes>
)


########################################################
# 自动化生成配置头文件
########################################################
configure_file (
  "${CMAKE_CURRENT_LIST_DIR}/qwt_version_info.h.in"
  "${CMAKE_CURRENT_LIST_DIR}/qwt_version_info.h"
)

########################################################
# 安装
########################################################
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

message(STATUS "${QWT_LIB_NAME} install dir is : ${CMAKE_INSTALL_PREFIX}")


# PACKAGE_PREFIX_DIR = ${CMAKE_CURRENT_LIST_DIR}/../..
# 最终变为${CMAKE_CURRENT_LIST_DIR}/../../include/qwt
set(QWT_LIB_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/${QWT_LIB_NAME})

# Generate library version info which will generate xxxConfigVersion.cmake,
# the ${PACKAGE_VERSION} is the version defined in project()

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${QWT_LIB_NAME}ConfigVersion.cmake
    VERSION ${QWT_LIB_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/QwtConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${QWT_LIB_NAME}Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${QWT_LIB_NAME}
  PATH_VARS QWT_LIB_INCLUDE_INSTALL_DIR
)
# 声明导出target的名称
install(TARGETS ${QWT_LIB_NAME}
    EXPORT ${QWT_LIB_NAME}Targets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${QWT_LIB_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT "${QWT_LIB_NAME}Targets"
    FILE ${QWT_LIB_NAME}Targets.cmake # 导出的文件基准名。
    NAMESPACE ${QWT_LIB_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${QWT_LIB_NAME}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${QWT_LIB_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${QWT_LIB_NAME}ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${QWT_LIB_NAME}
)

install(FILES
    ${QWT_HEADER_FILES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${QWT_LIB_NAME}
    COMPONENT headers
)

file(GLOB QWT_INCLUDE_IN_CLASS_FILES "${CMAKE_CURRENT_LIST_DIR}/../classincludes/Qwt*")
install(FILES
    ${QWT_INCLUDE_IN_CLASS_FILES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${QWT_LIB_NAME}
    COMPONENT headers
)

export(EXPORT ${QWT_LIB_NAME}Targets
   FILE ${CMAKE_CURRENT_BINARY_DIR}/${QWT_LIB_NAME}Targets.cmake
)
########################################################
# dll资源信息
########################################################
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/create_win32_resource_version.cmake)
if(WIN32)
        create_win32_resource_version(
                TARGET ${QWT_LIB_NAME}
                FILENAME ${QWT_LIB_NAME}
                VERSION ${QWT_LIB_VERSION}
                EXT "dll"
                COMPANYNAME "QWT | http://qwt.sf.net"
                COPYRIGHT ""
                DESCRIPTION ${QWT_LIB_DESCRIPTION}
        )
endif()
