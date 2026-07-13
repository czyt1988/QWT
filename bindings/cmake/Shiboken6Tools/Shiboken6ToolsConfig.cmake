# Shiboken6ToolsConfig.cmake  (VENDORED SHIM)
#
# This is NOT the upstream Shiboken6Tools config. The PySide6 / shiboken6-generator
# pip wheels (verified 6.7.3, 6.8.3) ship shiboken6.exe + libclang + the shiboken
# headers and PySide6 headers/typesystems, but ship NO CMake config and NO
# shiboken_generator_create_binding macro (those only exist in a source-built
# pyside-setup tree).
#
# This shim provides exactly the pieces bindings/CMakeLists.txt needs so it can be
# kept verbatim from the task brief:
#   * find_package(Shiboken6Tools REQUIRED) succeeds
#   * the shiboken_generator_create_binding(EXTENSION_TARGET ... GENERATED_SOURCES
#     ... HEADERS ... TYPESYSTEM_FILE ... LIBRARY_TARGET ... QT_MODULES Core Gui)
#     macro is defined
#   * imported targets Shiboken6::libshiboken and PySide6::pyside6 exist for linking
#
# The macro drives shiboken6.exe with the flag set validated by direct invocation;
# it mirrors the official PySide `create_pyside_module` command line
# (sources/pyside6/cmake/Macros/PySideModules.cmake).

set(SHIBOKEN6TOOLS_FOUND TRUE)

# Locate the pip-installed shiboken6_generator + PySide6 (paths were discovered by
# FindPySide6Shiboken.cmake). Fall back to a fresh lookup if the variables are unset.
if(NOT SHIBOKEN6_EXECUTABLE)
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c
        "import site,os; sp=next(p for p in site.getsitepackages() if 'site-packages' in p); d=os.path.join(sp,'shiboken6_generator'); print(d)"
        OUTPUT_VARIABLE _sb_pkg_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
    find_program(SHIBOKEN6_EXECUTABLE NAMES shiboken6 shiboken6.exe
        HINTS "${_sb_pkg_dir}" NO_DEFAULT_PATH)
endif()

# Imported targets for the generated wrapper .cpp to link against.
if(NOT TARGET Shiboken6::libshiboken)
    add_library(Shiboken6::libshiboken UNKNOWN IMPORTED)
    set_target_properties(Shiboken6::libshiboken PROPERTIES
        IMPORTED_LOCATION "${SHIBOKEN6_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${SHIBOKEN6_INCLUDE_DIR}")
endif()

if(NOT TARGET PySide6::pyside6)
    add_library(PySide6::pyside6 UNKNOWN IMPORTED)
    set_target_properties(PySide6::pyside6 PROPERTIES
        IMPORTED_LOCATION "${PYSIDE6_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${PYSIDE6_INCLUDE_DIR}")
endif()

# Collect the MSVC / Windows SDK STL include dirs that shiboken's libclang needs
# to find <type_traits>, <string>, etc. when parsing Qt/qwt headers. On a clean
# shell INCLUDE is empty, so we derive the dirs from the Visual Studio install.
# (Using the Windows ';' separator as shiboken's --include-paths expects.)
function(_shiboken_get_msvc_stl_includes out_var)
    set(_msvc_inc "")
    if(MSVC)
        # Prefer $ENV{INCLUDE} if it is populated (e.g. when configuring inside a
        # vcvars shell / VS IDE / cmake tools with env).
        if(DEFINED ENV{INCLUDE} AND NOT "$ENV{INCLUDE}" STREQUAL "")
            string(REPLACE "\\" "/" _inc "$ENV{INCLUDE}")
            string(REPLACE ";" ";" _msvc_inc "${_inc}")
        else()
            # Find the latest MSVC toolchain include + the Windows SDK ucrt/um/shared.
            set(_vs_root)
            foreach(_cand
                "C:/Program Files (x86)/Microsoft Visual Studio"
                "D:/Program Files (x86)/Microsoft Visual Studio"
                "C:/Program Files/Microsoft Visual Studio"
                "D:/Program Files/Microsoft Visual Studio")
                if(EXISTS "${_cand}")
                    set(_vs_root "${_cand}")
                    break()
                endif()
            endforeach()
            if(_vs_root)
                file(GLOB _msvc_tool_dirs "${_vs_root}/*/Community/VC/Tools/MSVC/*"
                                          "${_vs_root}/*/Professional/VC/Tools/MSVC/*"
                                          "${_vs_root}/*/Enterprise/VC/Tools/MSVC/*"
                                          "${_vs_root}/*/BuildTools/VC/Tools/MSVC/*")
                list(SORT _msvc_tool_dirs ORDER DESCENDING)
                if(_msvc_tool_dirs)
                    list(GET _msvc_tool_dirs 0 _msvc_tool)
                    set(_msvc_inc "${_msvc_tool}/include")
                endif()
                # Windows 10 SDK ucrt + um + shared (pick latest)
                file(GLOB _sdk_dirs "C:/Program Files (x86)/Windows Kits/10/include/*")
                list(SORT _sdk_dirs ORDER DESCENDING)
                if(_sdk_dirs)
                    list(GET _sdk_dirs 0 _sdk)
                    set(_msvc_inc "${_msvc_inc};${_sdk}/ucrt;${_sdk}/um;${_sdk}/shared")
                endif()
            endif()
        endif()
    endif()
    set(${out_var} "${_msvc_inc}" PARENT_SCOPE)
endfunction()

# The macro the task brief's bindings/CMakeLists.txt calls.
# Signature (from the official PySide examples):
#   shiboken_generator_create_binding(
#       EXTENSION_TARGET  <name>
#       GENERATED_SOURCES <cpp1;cpp2;...>
#       HEADERS           <global.h>
#       TYPESYSTEM_FILE   <typesystem.xml>
#       LIBRARY_TARGET    <cmake-lib-target>
#       QT_MODULES        Core [Gui ...]
#   )
macro(shiboken_generator_create_binding)
    set(_sb_opt "")
    set(_sb_one EXTENSION_TARGET HEADERS TYPESYSTEM_FILE LIBRARY_TARGET)
    set(_sb_multi GENERATED_SOURCES QT_MODULES GEN_DEFINES TYPESYSTEM_PATHS)
    cmake_parse_arguments(_sb "${_sb_opt}" "${_sb_one}" "${_sb_multi}" ${ARGN})
    if(_sb_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "shiboken_generator_create_binding: unknown arguments: ${_sb_UNPARSED_ARGUMENTS}")
    endif()

    set(_ext_target ${_sb_EXTENSION_TARGET})
    set(_gen_sources ${_sb_GENERATED_SOURCES})
    set(_global_header ${_sb_HEADERS})
    set(_typesystem ${_sb_TYPESYSTEM_FILE})
    set(_lib_target ${_sb_LIBRARY_TARGET})
    set(_qt_modules ${_sb_QT_MODULES})

    # Build the include path list: Qt module include dirs + the wrapped lib's
    # build interface includes + shiboken + PySide6 + Python + MSVC STL.
    set(_inc_dirs "")
    # Qt6::Core's INTERFACE_INCLUDE_DIRECTORIES are generator expressions that
    # do not resolve at configure time, so derive the Qt include root from Qt6_DIR
    # (== <prefix>/lib/cmake/Qt6  ->  prefix = Qt6_DIR/../../..).
    if(DEFINED Qt6_DIR)
        get_filename_component(_qt6_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
        list(APPEND _inc_dirs "${_qt6_prefix}/include")
        foreach(_mod ${_qt_modules})
            list(APPEND _inc_dirs "${_qt6_prefix}/include/Qt${_mod}")
        endforeach()
    endif()
    get_target_property(_lib_inc ${_lib_target} INTERFACE_INCLUDE_DIRECTORIES)
    if(_lib_inc)
        list(APPEND _inc_dirs ${_lib_inc})
    endif()
    list(APPEND _inc_dirs
        "${SHIBOKEN6_INCLUDE_DIR}"
        "${PYSIDE6_INCLUDE_DIR}"
        "${Python_INCLUDE_DIRS}")
    _shiboken_get_msvc_stl_includes(_msvc_stl_inc)
    if(_msvc_stl_inc)
        list(APPEND _inc_dirs ${_msvc_stl_inc})
    endif()
    list(REMOVE_DUPLICATES _inc_dirs)
    # Join the list into a single argument value. shiboken's --include-paths uses
    # ';' as the separator, but CMake re-splits list elements on ';' when expanding
    # a variable inside COMMAND. Escape each ';' as '\;' so CMake treats the whole
    # thing as ONE argument (it un-escapes '\;' back to ';' when invoking the tool).
    string(REPLACE ";" "\\;" _include_paths "${_inc_dirs}")

    # Typesystem search paths: PySide6's bundled typesystems + the binding dir
    # + any extra paths the caller supplies (e.g. the qwtcore dir so qwtplot
    # can load typesystem_qwtcore.xml via load-typesystem generate="no").
    get_filename_component(_ts_dir ${_typesystem} DIRECTORY)
    set(_typesystem_paths_list "${PYSIDE6_TYPESYSTEMS_DIR}" "${_ts_dir}")
    if(_sb_TYPESYSTEM_PATHS)
        list(APPEND _typesystem_paths_list ${_sb_TYPESYSTEM_PATHS})
    endif()
    list(REMOVE_DUPLICATES _typesystem_paths_list)
    string(REPLACE ";" "\\;" _typesystem_paths "${_typesystem_paths_list}")

    # Shiboken writes wrappers under <output-dir>/<package>/, where <package> is
    # the typesystem's package name (== _ext_target here). The brief's
    # GENERATED_SOURCES list expects them at ${CMAKE_CURRENT_BINARY_DIR}/<ext>/,
    # so point the output directory at ${CMAKE_CURRENT_BINARY_DIR} and let the
    # package subdir create the <ext>/ level.
    set(_out_dir ${CMAKE_CURRENT_BINARY_DIR})

    # Forward generation-time defines to shiboken's libclang parser.
    # `target_compile_definitions` only affects the MSVC compile of the generated
    # .cpp files, NOT shiboken's own parse of the headers. Headers like
    # qwt_series_data.h use QWT_PYTHON_WRAPPER to switch the QwtSeriesData<T>
    # pure-virtuals (size/sample/boundingRect) to non-pure-virtual default impls;
    # if the define is absent at parse time the methods stay pure-virtual and the
    # abstract template base cannot be subclassed from Python. Pass each define
    # through shiboken's --clang-option so libclang sees it while parsing.
    # Generation-time defines forwarded to shiboken's libclang parser.
    # Caller passes module-specific defines via GEN_DEFINES; merge with the
    # always-required QWT_PYTHON_WRAPPER and QT_NO_KEYWORDS.
    set(_gen_defines QWT_PYTHON_WRAPPER QT_NO_KEYWORDS)
    if(_sb_GEN_DEFINES)
        list(APPEND _gen_defines ${_sb_GEN_DEFINES})
    endif()
    list(REMOVE_DUPLICATES _gen_defines)
    set(_clang_defines "")
    foreach(_d ${_gen_defines})
        list(APPEND _clang_defines "--clang-option=-D${_d}")
    endforeach()

    set(_shiboken_command
        ${SHIBOKEN6_EXECUTABLE}
        --generator-set=shiboken
        "--include-paths=${_include_paths}"
        "--typesystem-paths=${_typesystem_paths}"
        --output-directory=${_out_dir}
        --enable-pyside-extensions
        --avoid-protected-hack
        --lean-headers
        ${_clang_defines}
        ${_global_header}
        ${_typesystem}
    )

    # Generation step: the wrapper .cpp files are the declared OUTPUT so CMake
    # runs shiboken before compiling them. shiboken writes everything under
    # <out_dir>/<package>/ where package == _ext_target.
    add_custom_command(
        OUTPUT ${_gen_sources} "${_out_dir}/${_ext_target}/mjb_rejected_classes.log"
        COMMAND ${_shiboken_command}
        DEPENDS ${_global_header} ${_typesystem}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running shiboken6 generator for ${_ext_target}"
        VERBATIM
    )

    # The extension module itself (a .pyd on Windows).
    add_library(${_ext_target} MODULE ${_gen_sources})

    target_link_libraries(${_ext_target} PRIVATE
        ${_lib_target}
        Shiboken6::libshiboken
        PySide6::pyside6
    )
    # The generated qwtcore_python.h #includes <pyside6_qtcore_python.h> etc.
    # which live under PySide6/include/<QtModule>/. Add those dirs so the
    # wrapper .cpp compiles (the PySide6::pyside6 imported target only exposes
    # the PySide6/include root).
    foreach(_mod ${_qt_modules})
        if(EXISTS "${PYSIDE6_INCLUDE_DIR}/Qt${_mod}")
            target_include_directories(${_ext_target} PRIVATE
                "${PYSIDE6_INCLUDE_DIR}/Qt${_mod}")
        endif()
    endforeach()
    # The generated wrappers #include <Python.h> (via shiboken's sbkpython.h),
    # so the Python include dir must be on the compile path. Prefer the
    # Python::Module target if present (find_package(Python Development)), else
    # fall back to the Python_INCLUDE_DIRS variable.
    if(TARGET Python::Module)
        target_link_libraries(${_ext_target} PRIVATE Python::Module)
    elseif(Python_INCLUDE_DIRS)
        target_include_directories(${_ext_target} PRIVATE ${Python_INCLUDE_DIRS})
    endif()
    # Qt modules are NOT linked explicitly here: the wrapped LIBRARY_TARGET
    # (e.g. qwt::core) already links Qt6::Core/Qt6::Gui PUBLIC, so the binding
    # target inherits them transitively (mirrors the official widgetbinding
    # example, which links only the wiggly lib, not Qt directly).

    # shiboken module init needs this define; matches upstream examples.
    target_compile_definitions(${_ext_target} PRIVATE
        HAVE_SHIBOKEN6
        QT_NO_KEYWORDS
    )

    set_target_properties(${_ext_target} PROPERTIES
        PREFIX ""
        AUTOMOC OFF)
    if(WIN32)
        set_target_properties(${_ext_target} PROPERTIES SUFFIX ".pyd")
    endif()
endmacro()
