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
# Explicit list (lowercased class names) — expand in later tasks.
set(_qwt_generated_sources
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplot_module_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwttext_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotabstractcanvas_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotdict_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplotcanvas_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${_qwt_bindings_lib}/qwtplot_wrapper.cpp
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

# Generation-time defines: make shiboken see the python-wrapper hooks + import-side exports.
target_compile_definitions(${_qwt_bindings_lib} PRIVATE
    QWT_PYTHON_WRAPPER
    QWT_DLL
    QWTCORE_DLL
    QT_NO_KEYWORDS
)

# The generated qwtplot_python.h #includes "qwtcore_python.h" (produced by the
# qwtcore binding generation step) to reference core type converters. Add the
# qwtcore generated output dir to the include path.
target_include_directories(${_qwt_bindings_lib} PRIVATE
    ${CMAKE_BINARY_DIR}/bindings/core/qwtcore
    ${CMAKE_BINARY_DIR}/bindings/plot/qwtplot
)

# Output next to the qwt DLLs so the test harness can find both.
set_target_properties(${_qwt_bindings_lib} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
