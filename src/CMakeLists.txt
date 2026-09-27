cmake_minimum_required(VERSION 3.10)

########################################################
# Qt
########################################################
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
# version file
########################################################
configure_file (
  "${CMAKE_CURRENT_LIST_DIR}/qwt_version_info.h.in"
  "${CMAKE_CURRENT_LIST_DIR}/qwt_version_info.h"
)

########################################################
# qwt core (shared color/palette utilities)
########################################################
add_subdirectory(core)

########################################################
# qwt
########################################################
add_subdirectory(plot)

########################################################
# qwt3d
########################################################
if(QWT_CONFIG_QWTPLOT_3D)
    add_subdirectory(plot3d)
endif()
