# qwt
cmake_minimum_required(VERSION 3.5)

set(QWTPLOT_LIB_NAME "plot")
set(QWTPLOT_LIB_DESCRIPTION "qwt::plot | 2D scientific plotting library")

message("")
message("${QWT_LIB_NAME} ${QWT_VERSION} | ${QWTPLOT_LIB_DESCRIPTION}")

project(${QWTPLOT_LIB_NAME}
        VERSION ${QWT_VERSION}
        LANGUAGES CXX
        DESCRIPTION "${QWTPLOT_LIB_DESCRIPTION}"
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/moc)
########################################################
# files 
########################################################
set(QWTPLOT_HEADER_BASE
        qwt_abstract_scale_draw.h
        qwt_colormap_compat.h
        qwt_column_symbol.h
        qwt_date_scale_draw.h
        qwt_date_scale_engine.h
        qwt_dyngrid_layout.h
        qwt_graphic.h
        qwt_interval_symbol.h
        qwt_magnifier.h
        qwt_null_paintdevice.h
        qwt_painter.h
        qwt_painter_command.h
        qwt_cache_panner.h
        qwt_picker.h
        qwt_picker_machine.h
        qwt_pixel_matrix.h
        qwt_round_scale_draw.h
        qwt_scale_draw.h
        qwt_text_scale_draw.h
        qwt_spline.h
        qwt_spline_basis.h
        qwt_spline_parametrization.h
        qwt_spline_local.h
        qwt_spline_cubic.h
        qwt_spline_pleasing.h
        qwt_spline_polynomial.h
        qwt_symbol.h
        qwt_stylesheet_recorder.h
        qwt_text_engine.h
        qwt_text_label.h
        qwt_text.h
        qwt_widget_overlay.h
        qwt_utils.h
        # forward headers for core-migrated files
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
        qwt_grid_data.hpp
        qwt_scale_engine.h
)
set(QWTPLOT_SOURCE_BASE
        qwt_abstract_scale_draw.cpp
        qwt_column_symbol.cpp
        qwt_date_scale_draw.cpp
        qwt_date_scale_engine.cpp
        qwt_dyngrid_layout.cpp
        qwt_event_pattern.cpp
        qwt_graphic.cpp
        qwt_interval_symbol.cpp
        qwt_magnifier.cpp
        qwt_null_paintdevice.cpp
        qwt_painter.cpp
        qwt_painter_command.cpp
        qwt_cache_panner.cpp
        qwt_picker.cpp
        qwt_picker_machine.cpp
        qwt_pixel_matrix.cpp
        qwt_round_scale_draw.cpp
        qwt_scale_draw.cpp
        qwt_text_scale_draw.cpp
        qwt_spline.cpp
        qwt_spline_basis.cpp
        qwt_spline_parametrization.cpp
        qwt_spline_local.cpp
        qwt_spline_cubic.cpp
        qwt_spline_pleasing.cpp
        qwt_spline_polynomial.cpp
        qwt_symbol.cpp
        qwt_stylesheet_recorder.cpp
        qwt_text_engine.cpp
        qwt_text_label.cpp
        qwt_text.cpp
        qwt_widget_overlay.cpp
        qwt_utils.cpp
)

set(QWTPLOT_HEADER_FILES ${QWTPLOT_HEADER_BASE})
set(QWTPLOT_SOURCE_FILES ${QWTPLOT_SOURCE_BASE})

if(QWT_CONFIG_QWTPLOT)
    set(QWTPLOT_HEADER_PLOT
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
        qwt_plot_boxchart.h
        qwt_plot_layout_engine.h
        qwt_plot_layout.h
        qwt_parasite_plot_layout.h
        qwt_plot_scale_event_dispatcher.h
        qwt_plot_marker.h
        qwt_plot_arrowmarker.h
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
        qwt_plot_series_data_picker_group.h
        qwt_plot_axis_zoomer.h
        qwt_plot_canvas_zoomer.h
        qwt_plot_magnifier.h
        qwt_plot_axis_wheel_interaction.h
        qwt_plot_rescaler.h
        qwt_point_mapper.h
        qwt_vectorfield_symbol.h
        qwt_sampling_thread.h
        # forward headers for core-migrated files
        qwt_box_statistics.h
        qwt_simd_argminmax.h
        qwt_grid_data.hpp
        qwt_samples.h
        qwt_series_data.h
        qwt_series_store.h
        qwt_point_data.h
        qwt_raster_data.h
        qwt_matrix_raster_data.h
        qwt_grid_raster_data.h
        qwt_scale_widget.h
        qwt_figure_layout.h
        qwt_figure.h
        qwt_figure_widget_overlay.h
        # utility classes
        qwt_plot_item_info.h
        qwt_plot_data_access.h
        qwt_plot_factory.h
        qwt_plot_transform.h
        qwt_plot_styling.h
        qwt_pyplot.h
    )
    set(QWTPLOT_SOURCE_PLOT
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
        qwt_plot_boxchart.cpp
        qwt_plot_spectrogram.cpp
        qwt_plot_spectrocurve.cpp
        qwt_plot_scaleitem.cpp
        qwt_plot_legenditem.cpp
        qwt_plot_seriesitem.cpp
        qwt_plot_shapeitem.cpp
        qwt_plot_vectorfield.cpp
        qwt_plot_marker.cpp
        qwt_plot_arrowmarker.cpp
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
        qwt_plot_series_data_picker_group.cpp
        qwt_plot_axis_zoomer.cpp
        qwt_plot_canvas_zoomer.cpp
        qwt_plot_magnifier.cpp
        qwt_plot_axis_wheel_interaction.cpp
        qwt_plot_rescaler.cpp
        qwt_point_mapper.cpp
        qwt_vectorfield_symbol.cpp
        qwt_sampling_thread.cpp
        qwt_scale_widget.cpp
        qwt_figure_layout.cpp
        qwt_figure.cpp
        qwt_figure_widget_overlay.cpp
        # utility classes
        qwt_plot_item_info.cpp
        qwt_plot_data_access.cpp
        qwt_plot_factory.cpp
        qwt_plot_transform.cpp
        qwt_plot_styling.cpp
        qwt_pyplot.cpp
    )

    list(APPEND QWTPLOT_HEADER_FILES ${QWTPLOT_HEADER_PLOT})
    list(APPEND QWTPLOT_SOURCE_FILES ${QWTPLOT_SOURCE_PLOT})
endif()

if(QWT_CONFIG_QWTOPENGL)
    set(QWTPLOT_HEADER_OPENGL
        qwt_plot_opengl_canvas.h
    )
    set(QWTPLOT_SOURCE_OPENGL
        qwt_plot_opengl_canvas.cpp
    )
    if(${QT_VERSION_MAJOR} LESS 6)
        list(APPEND QWTPLOT_HEADER_OPENGL qwt_plot_glcanvas.h)
        list(APPEND QWTPLOT_SOURCE_OPENGL qwt_plot_glcanvas.cpp)
    endif()
    list(APPEND QWTPLOT_HEADER_FILES ${QWTPLOT_HEADER_OPENGL})
    list(APPEND QWTPLOT_SOURCE_FILES ${QWTPLOT_SOURCE_OPENGL})
endif()

if(QWT_CONFIG_QWTSVG)
    set(QWTPLOT_HEADER_SVG
        qwt_plot_svgitem.h
    )
    set(QWTPLOT_SOURCE_SVG
        qwt_plot_svgitem.cpp
    )
    list(APPEND QWTPLOT_HEADER_FILES ${QWTPLOT_HEADER_SVG})
    list(APPEND QWTPLOT_SOURCE_FILES ${QWTPLOT_SOURCE_SVG})
endif()

if(QWT_CONFIG_QWTPOLAR)
    set(QWTPLOT_HEADER_POLAR
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
    set(QWTPLOT_SOURCE_POLAR
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
    list(APPEND QWTPLOT_HEADER_FILES ${QWTPLOT_HEADER_POLAR})
    list(APPEND QWTPLOT_SOURCE_FILES ${QWTPLOT_SOURCE_POLAR})
endif()


if(QWT_CONFIG_QWTWIDGETS)
    set(QWTPLOT_HEADER_WIDGETS
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
    set(QWTPLOT_SOURCE_WIDGETS
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
    list(APPEND QWTPLOT_HEADER_FILES ${QWTPLOT_HEADER_WIDGETS})
    list(APPEND QWTPLOT_SOURCE_FILES ${QWTPLOT_SOURCE_WIDGETS})
endif()


add_library(${QWTPLOT_LIB_NAME} SHARED
        ${QWTPLOT_HEADER_FILES}
        ${QWTPLOT_SOURCE_FILES}
)

add_library(${QWT_LIB_NAME}::${QWTPLOT_LIB_NAME} ALIAS ${QWTPLOT_LIB_NAME})


########################################################
# Dependency
########################################################
target_link_libraries(${QWTPLOT_LIB_NAME} PUBLIC
    ${QWT_LIB_NAME}::core
)
target_link_libraries(${QWTPLOT_LIB_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)
target_link_libraries(${QWTPLOT_LIB_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::PrintSupport
)
if(QWT_CONFIG_QWTOPENGL)
    find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
        OpenGL
        REQUIRED
    )
    target_link_libraries(${QWTPLOT_LIB_NAME} PUBLIC
        Qt${QT_VERSION_MAJOR}::OpenGL
    )
    if(${QT_VERSION_MAJOR} EQUAL 6)
        find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
            OpenGLWidgets
            REQUIRED
        )
        target_link_libraries(${QWTPLOT_LIB_NAME} PUBLIC
            Qt${QT_VERSION_MAJOR}::OpenGLWidgets
        )
        message(STATUS "target link Qt${QT_VERSION_MAJOR}::OpenGLWidgets")
    endif()
endif()

if(QWT_CONFIG_QWTSVG)
    find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
        Svg
        REQUIRED
    )
    target_link_libraries(${QWTPLOT_LIB_NAME} PUBLIC
        Qt${QT_VERSION_MAJOR}::Svg
    )
endif()

########################################################
# target properties
########################################################
set_target_properties(${QWTPLOT_LIB_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    VERSION ${QWT_VERSION}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "${QWT_LIB_NAME}${QWTPLOT_LIB_NAME}"
)

target_include_directories(${QWTPLOT_LIB_NAME} PUBLIC
    $<INSTALL_INTERFACE:include/${QWT_LIB_NAME}> # version_file
    $<INSTALL_INTERFACE:include/${QWT_LIB_NAME}/${QWTPLOT_LIB_NAME}>
    $<INSTALL_INTERFACE:include/${QWT_LIB_NAME}/classincludes>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..># version_file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../classincludes>
)

target_compile_definitions(${QWTPLOT_LIB_NAME} PRIVATE QT_DLL)
target_compile_definitions(${QWTPLOT_LIB_NAME} PUBLIC QWT_DLL)
target_compile_definitions(${QWTPLOT_LIB_NAME} PRIVATE QWT_MAKEDLL)

########################################################
# install 
########################################################
include(GNUInstallDirs)
install(TARGETS ${QWTPLOT_LIB_NAME} # plot
    EXPORT ${QWT_TARGET_NAME}       # qwtTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES
    ${QWTPLOT_HEADER_FILES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${QWT_LIB_NAME}/${QWTPLOT_LIB_NAME}
    COMPONENT headers
)

########################################################
# dll资源信息
########################################################
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/create_win32_resource_version.cmake)
if(WIN32)
        create_win32_resource_version(
                TARGET ${QWTPLOT_LIB_NAME}
                FILENAME ${QWTPLOT_LIB_NAME}
                VERSION ${QWT_VERSION}
                EXT "dll"
                COMPANYNAME "QWT | http://qwt.sf.net"
                COPYRIGHT ""
                DESCRIPTION ${QWTPLOT_LIB_DESCRIPTION}
        )
endif()
