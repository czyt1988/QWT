# bindings/plot/CMakeLists.txt
# PySide6 / Shiboken6 bindings for Qwt 2D plotting (qwtplot module).

set(_qwt_bindings_lib "qwtplot")
set(_qwt_global_header ${CMAKE_CURRENT_SOURCE_DIR}/global.h)
set(_qwt_typesystem ${CMAKE_CURRENT_SOURCE_DIR}/typesystem_qwtplot.xml)

# PySide6 include dirs: plot needs QtCore + QtGui + QtWidgets.
foreach(_inc_dir ${pyside_include_dir})
    list(APPEND _qwt_pyside_includes "${_inc_dir}/QtCore")
    list(APPEND _qwt_pyside_includes "${_inc_dir}/QtGui")
    list(APPEND _qwt_pyside_includes "${_inc_dir}/QtWidgets")
endforeach()

# Shiboken generates one wrapper .cpp per type + one module wrapper.
# Explicit list (lowercased class names).
set(_qwt_generated_sources
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplot_module_wrapper.cpp
    # Value types
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwttext_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtcolumnrect_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtintervalsymbol_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtgraphic_wrapper.cpp
    # Object types — base/mixin
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtnullpaintdevice_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtabstractseriesstore_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotabstractcanvas_wrapper.cpp
    # Object types — plot widget & canvas
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotdict_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotcanvas_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplottransparentcanvas_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplot_wrapper.cpp
    # Object types — axes / scale draw
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtaxis_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtabstractscaledraw_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtscaledraw_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtroundscaledraw_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtdatescaledraw_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtdatescaleengine_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtscalewidget_wrapper.cpp
    # Object types — plot items (base + non-series)
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotitem_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotseriesitem_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotgrid_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotmarker_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotarrowmarker_wrapper.cpp
    # Object types — symbols
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsymbol_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtcolumnsymbol_wrapper.cpp
    # Object types — text label
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwttextlabel_wrapper.cpp
    # Object types — layout / renderer / painter
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotlayout_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotrenderer_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotdirectpainter_wrapper.cpp
    # Object types — curve fitters
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtcurvefitter_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsplinecurvefitter_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtweedingcurvefitter_wrapper.cpp
    # Task 3: raster items
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotrasteritem_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotspectrogram_wrapper.cpp
    # Task 3: series items
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotcurve_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplothistogram_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotabstractbarchart_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotbarchart_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotmultibarchart_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotintervalcurve_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplottradingcurve_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotboxchart_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotspectrocurve_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotvectorfield_wrapper.cpp
    # Task 3: vector field symbols
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtvectorfieldsymbol_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtvectorfieldarrow_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtvectorfieldthinarrow_wrapper.cpp
    # Task 3: non-series items
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotshapeitem_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotzoneitem_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplottextlabel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotscaleitem_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotgraphicitem_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotlegenditem_wrapper.cpp
    # Task 3: legends
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtabstractlegend_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtlegend_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtlegenddata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtlegendlabel_wrapper.cpp
    # Task 4: event pattern + picker machines
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwteventpattern_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpicker_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpickermachine_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpickertrackermachine_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpickerclickpointmachine_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpickerdragpointmachine_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpickerclickrectmachine_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpickerdragrectmachine_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpickerdraglinemachine_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpickerpolygonmachine_wrapper.cpp
    # Task 4: pickers
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotpicker_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtcanvaspicker_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotseriesdatapicker_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotseriesdatapickergroup_wrapper.cpp
    # Task 4: zoomers / panners / magnifier
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotaxiszoomer_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotcanvaszoomer_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotpanner_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotcachepanner_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtcachepanner_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtmagnifier_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotmagnifier_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotrescaler_wrapper.cpp
    # Task 4: overlays
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtwidgetoverlay_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtfigurewidgetoverlay_wrapper.cpp
    # Task 4: figure / parasite framework
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtfigure_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtparasiteplotlayout_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotscaleeventdispatcher_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsamplingthread_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotlayoutengine_wrapper.cpp
    # Task 4: utility static classes
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotfactory_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotiteminfo_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotdataaccess_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotstyling_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplottransform_wrapper.cpp
    # Task 4: splines
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtspline_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsplinebasis_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsplinecubic_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsplinelocal_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsplinepleasing_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsplineparametrization_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsplinepolynomial_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpointmapper_wrapper.cpp
)

shiboken_generator_create_binding(
    EXTENSION_TARGET  ${_qwt_bindings_lib}
    GENERATED_SOURCES ${_qwt_generated_sources}
    HEADERS           ${_qwt_global_header}
    TYPESYSTEM_FILE   ${_qwt_typesystem}
    LIBRARY_TARGET    qwt::plot
    QT_MODULES        Core Gui Widgets
    GEN_DEFINES       QWT_DLL QWTCORE_DLL
    TYPESYSTEM_PATHS  ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

# Hand-written glue source for series item size()/sample() workarounds.
target_sources(${_qwt_bindings_lib} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glue_plot.cpp)

# Generation-time defines: make shiboken see the python-wrapper hooks + import-side exports.
target_compile_definitions(${_qwt_bindings_lib} PRIVATE
    QWT_PYTHON_WRAPPER
    QWT_DLL
    QWTCORE_DLL
    QT_NO_KEYWORDS
)

# The generated qwtplot_python.h #includes "qwtcore_python.h" (produced by the
# qwtcore binding generation step) to reference core type converters. Add the
# qwtcore generated output dir + the binding source dir (for glue_plot.h) to
# the include path.
target_include_directories(${_qwt_bindings_lib} PRIVATE
    ${CMAKE_BINARY_DIR}/bindings/core/qwtcore
    ${CMAKE_BINARY_DIR}/bindings/plot/qwtplot
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Output next to the qwt DLLs so the test harness can find both.
set_target_properties(${_qwt_bindings_lib} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
