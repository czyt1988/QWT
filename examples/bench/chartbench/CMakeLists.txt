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
# QIm - optional extra backend, kept in the repository as a git submodule
# (https://gitee.com/czyt1988/qim).
#
# QIm is optional by design: the adapter is compiled only when the sources are
# present.  When the submodule has not been checked out, the download below is
# attempted once - non-interactively and with a timeout - so that an offline
# configure neither blocks on credentials nor fails.  If every attempt fails
# the adapter is dropped and everything else still builds.
# ---------------------------------------------------------------------------
set(CHARTBENCH_QIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qim"
    CACHE PATH "chartbench: QIm source directory")
option(CHARTBENCH_QIM_AUTOFETCH
       "chartbench: download QIm with git when the submodule is not checked out" ON)
option(CHARTBENCH_QIM_FETCH_ATTEMPTED
       "chartbench: automatic QIm download already attempted (set to OFF to retry)" OFF)
set(CHARTBENCH_QIM_URL "https://gitee.com/czyt1988/qim.git"
    CACHE STRING "chartbench: repository used to download QIm")
set(CHARTBENCH_QIM_FETCH_TIMEOUT "60"
    CACHE STRING "chartbench: seconds to wait for the QIm download")

# Runs git with prompts disabled: a missing network has to return, not wait for
# a password.  out_result is "0" on success, otherwise git's exit code or a
# description of the failure (for example a timeout).
function(chartbench_git out_result out_log)
    set(_saved_prompt "$ENV{GIT_TERMINAL_PROMPT}")
    set(_saved_gcm "$ENV{GCM_INTERACTIVE}")
    set(ENV{GIT_TERMINAL_PROMPT} "0")
    set(ENV{GCM_INTERACTIVE} "never")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -c credential.interactive=false ${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
        TIMEOUT "${CHARTBENCH_QIM_FETCH_TIMEOUT}"
    )
    set(ENV{GIT_TERMINAL_PROMPT} "${_saved_prompt}")
    set(ENV{GCM_INTERACTIVE} "${_saved_gcm}")
    set(${out_result} "${result}" PARENT_SCOPE)
    set(${out_log} "${output}${error}" PARENT_SCOPE)
endfunction()

# Prints git's message on one line, truncated, so a failed download is
# diagnosable without flooding the configure output.
function(chartbench_print_git_log log)
    if(NOT log)
        return()
    endif()
    string(REGEX REPLACE "[\r\n]+" " | " log "${log}")
    string(STRIP "${log}" log)
    string(LENGTH "${log}" length)
    if(length GREATER 300)
        string(SUBSTRING "${log}" 0 300 log)
        string(APPEND log " ...")
    endif()
    message(STATUS "chartbench: git said: ${log}")
endfunction()

# Brings a QIm checkout into place, in order of preference:
#   1. the pinned submodule commit (git submodule update), which keeps the
#      library and the adapter in sync and is what a manual
#      "git submodule update --init" would produce,
#   2. a fresh clone of CHARTBENCH_QIM_URL - for release archives without a
#      .git directory, or when CHARTBENCH_QIM_DIR points somewhere else,
#   3. giving up (out_ok = FALSE), which merely disables the adapter.
# Never raises a CMake error.
function(chartbench_fetch_qim out_ok out_source)
    set(${out_ok} FALSE PARENT_SCOPE)

    find_package(Git QUIET)
    if(NOT GIT_EXECUTABLE)
        message(STATUS "chartbench: git not found - QIm cannot be downloaded")
        return()
    endif()

    set(_source_root "${CMAKE_CURRENT_LIST_DIR}/../../..")

    set(_in_git_checkout FALSE)
    if(EXISTS "${_source_root}/.git" AND EXISTS "${_source_root}/.gitmodules")
        set(_in_git_checkout TRUE)
    endif()

    # A clone into the default submodule path of a git checkout would leave
    # that tree dirty, so there the fallback clone goes into the build
    # directory instead.  An explicitly redirected CHARTBENCH_QIM_DIR is used
    # as given - the user asked for that path.
    set(_qim_dir_is_default FALSE)
    if(CHARTBENCH_QIM_DIR STREQUAL "${CMAKE_CURRENT_LIST_DIR}/qim")
        set(_qim_dir_is_default TRUE)
    endif()

    # 1) Pinned submodule commit.
    if(_in_git_checkout)
        file(RELATIVE_PATH _submodule_path "${_source_root}" "${CHARTBENCH_QIM_DIR}")
        # A redirected CHARTBENCH_QIM_DIR cannot be a submodule of this tree.
        if(NOT _submodule_path MATCHES "^\\.\\." AND NOT _submodule_path MATCHES "^[A-Za-z]:/")
            message(STATUS "chartbench: checking out the QIm submodule")
            chartbench_git(_result _log -C "${_source_root}"
                           submodule update --init --recursive --depth 1 --quiet
                           -- "${_submodule_path}")
            if(EXISTS "${CHARTBENCH_QIM_DIR}/CMakeLists.txt")
                set(${out_ok} TRUE PARENT_SCOPE)
                set(${out_source} "${CHARTBENCH_QIM_DIR}" PARENT_SCOPE)
                return()
            endif()
            message(STATUS "chartbench: QIm submodule checkout failed (${_result})")
            chartbench_print_git_log("${_log}")
        endif()
    endif()

    # 2) Plain clone.
    set(_clone_dir "${CHARTBENCH_QIM_DIR}")
    # Cleaning up after a failed clone is only allowed for a directory this
    # configure created itself: one that does not exist yet, or the empty
    # leftover of an uninitialised submodule.  Anything else is somebody's
    # data and is left alone.
    set(_may_delete FALSE)
    if(NOT EXISTS "${_clone_dir}")
        set(_may_delete TRUE)
    else()
        file(GLOB _entries "${_clone_dir}/*" "${_clone_dir}/.[!.]*")
        if(NOT _entries)
            set(_may_delete TRUE)
        endif()
    endif()
    if(_in_git_checkout AND _qim_dir_is_default)
        set(_clone_dir "${CMAKE_BINARY_DIR}/_deps/chartbench-qim")
        file(REMOVE_RECURSE "${_clone_dir}")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_deps")
        set(_may_delete TRUE)
    endif()

    message(STATUS "chartbench: cloning QIm from ${CHARTBENCH_QIM_URL}")
    chartbench_git(_result _log clone --depth 1 --single-branch --quiet
                   "${CHARTBENCH_QIM_URL}" "${_clone_dir}")
    if(_result STREQUAL "0" AND EXISTS "${_clone_dir}/CMakeLists.txt")
        set(${out_ok} TRUE PARENT_SCOPE)
        set(${out_source} "${_clone_dir}" PARENT_SCOPE)
        return()
    endif()

    message(STATUS "chartbench: QIm clone failed (${_result})")
    chartbench_print_git_log("${_log}")
    if(_may_delete)
        file(REMOVE_RECURSE "${_clone_dir}")
    endif()
endfunction()

set(CHARTBENCH_QIM_SOURCE "${CHARTBENCH_QIM_DIR}")

if(EXISTS "${CHARTBENCH_QIM_DIR}/CMakeLists.txt")
    # Already checked out - nothing to do.
elseif(CHARTBENCH_QIM_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/qim"
       AND EXISTS "${CMAKE_BINARY_DIR}/_deps/chartbench-qim/CMakeLists.txt")
    # A previous configure of this build tree downloaded QIm here.
    set(CHARTBENCH_QIM_SOURCE "${CMAKE_BINARY_DIR}/_deps/chartbench-qim")
elseif(NOT CHARTBENCH_QIM_AUTOFETCH)
    message(STATUS "chartbench: QIm sources missing and CHARTBENCH_QIM_AUTOFETCH=OFF - QIm adapter disabled")
elseif(CHARTBENCH_QIM_FETCH_ATTEMPTED)
    message(STATUS "chartbench: QIm sources missing - download not retried in this build tree")
    message(STATUS "chartbench: pass -DCHARTBENCH_QIM_FETCH_ATTEMPTED=OFF to try again, "
                   "or -DCHARTBENCH_QIM_DIR=<path> to use a local checkout")
else()
    # Remember the attempt so an offline reconfigure does not pay the timeout
    # again; a new build directory, or resetting the option, retries.
    set(CHARTBENCH_QIM_FETCH_ATTEMPTED ON CACHE BOOL
        "chartbench: automatic QIm download already attempted (set to OFF to retry)" FORCE)
    chartbench_fetch_qim(_qim_ok _qim_source)
    if(_qim_ok)
        set(CHARTBENCH_QIM_SOURCE "${_qim_source}")
        message(STATUS "chartbench: QIm sources ready (${CHARTBENCH_QIM_SOURCE})")
    else()
        message(STATUS "chartbench: QIm unavailable - QIm adapter disabled, everything else still builds")
        message(STATUS "chartbench: pass -DCHARTBENCH_QIM_FETCH_ATTEMPTED=OFF to retry the download, "
                       "or -DCHARTBENCH_QIM_DIR=<path> to use a local checkout")
    endif()
endif()

if(EXISTS "${CHARTBENCH_QIM_SOURCE}/CMakeLists.txt")
    set(QIM_BUILD_EXAMPLES OFF CACHE BOOL "chartbench: QIm examples off" FORCE)
    set(QIM_ENABLE_BENCHMARK OFF CACHE BOOL "chartbench: QIm benchmark off" FORCE)
    set(QIM_BUILD_TESTS OFF CACHE BOOL "chartbench: QIm tests off" FORCE)
    set(QIM_BUILD_QML OFF CACHE BOOL "chartbench: QIm QML layer off" FORCE)
    add_subdirectory("${CHARTBENCH_QIM_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/qim")
    set(CHARTBENCH_WITH_QIM ON)
else()
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
        DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
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
