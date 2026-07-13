# bindings/core/CMakeLists.txt
# PySide6 / Shiboken6 bindings for Qwt core (qwtcore module: all core value/object types).

set(_qwt_bindings_lib "qwtcore")
set(_qwt_global_header ${CMAKE_CURRENT_SOURCE_DIR}/global.h)
set(_qwt_typesystem ${CMAKE_CURRENT_SOURCE_DIR}/typesystem_qwtcore.xml)

# PySide6 include dirs (set by Shiboken6Tools as pyside_include_dir).
# For core we only need QtCore + QtGui headers on the parse side.
foreach(_inc_dir ${pyside_include_dir})
    list(APPEND _qwt_pyside_includes "${_inc_dir}/QtCore")
    list(APPEND _qwt_pyside_includes "${_inc_dir}/QtGui")
endforeach()

# Shiboken generates one wrapper .cpp per type + one module wrapper.
# Explicit list (lowercased class names) per controller resolution: the glob
# approach would be empty on first configure because shiboken_generator_create_binding
# needs the sources for the MODULE target at configure time. Reconcile against
# the actual files in build/bindings/qwtcore/ after the first generation run.
set(_qwt_generated_sources
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtcore_module_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtinterval_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpoint3d_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpointpolar_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtscalediv_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtscalemap_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtstatisticalsample_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtintervalsample_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsetsample_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtohlcsample_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtvectorfieldsample_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtboxsample_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtboxoutliersample_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtbezier_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtcolormap_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtlinearcolormap_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtalphacolormap_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwthuecolormap_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsaturationvaluecolormap_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtcolormappreset_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtcolorcycle_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwttransform_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtnulltransform_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtlogtransform_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpowertransform_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtscalearithmetic_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtscaleengine_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtlinearscaleengine_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtlogscaleengine_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpointseriesdata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtpoint3dseriesdata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtintervalseriesdata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsetseriesdata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtvectorfielddata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwttradingchartdata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtboxchartdata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtboxoutlierchartdata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsyntheticpointdata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtrasterdata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtmatrixrasterdata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtgridrasterdata_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtboxstatisticscalculator_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtsystemclock_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtdate_wrapper.cpp
)

shiboken_generator_create_binding(
    EXTENSION_TARGET  ${_qwt_bindings_lib}
    GENERATED_SOURCES ${_qwt_generated_sources}
    HEADERS           ${_qwt_global_header}
    TYPESYSTEM_FILE   ${_qwt_typesystem}
    LIBRARY_TARGET    qwt::core
    QT_MODULES        Core Gui
    GEN_DEFINES       QWTCORE_DLL
)

# Generation-time defines: make shiboken see the python-wrapper hooks + import-side exports.
target_compile_definitions(${_qwt_bindings_lib} PRIVATE
    QWT_PYTHON_WRAPPER
    QWTCORE_DLL
    QT_NO_KEYWORDS
)

# Output next to the qwt DLLs so the test harness can find both.
set_target_properties(${_qwt_bindings_lib} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
