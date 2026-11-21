cmake_minimum_required(VERSION 3.10)
SET(VERSION_SHORT 0.1)
SET(QWT_APP_NAME figureGrid)
project(${QWT_APP_NAME} VERSION ${VERSION_SHORT})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# qt library loading, minimum 5.8
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.8 COMPONENTS Core Gui Widgets REQUIRED)
file(GLOB APP_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB APP_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
# Shared flat SVG toolbar icons, see examples/common/icons/README.md
set(APP_ICON_RESOURCES "${CMAKE_CURRENT_LIST_DIR}/../../common/icons/icons.qrc")
add_executable(${QWT_APP_NAME}
    ${APP_HEADER_FILES}
    ${APP_SOURCE_FILES}
    ${APP_ICON_RESOURCES}
)

if(NOT TARGET qwt::plot)
    # the example is configured standalone, use an installed qwt
    message(STATUS "NOT TARGET qwt::plot find_package(qwt REQUIRED)")
    find_package(qwt REQUIRED)
endif()

target_link_libraries(${QWT_APP_NAME} PUBLIC qwt::plot)

set_target_properties(${QWT_APP_NAME} PROPERTIES
    AUTOMOC ON
    CXX_EXTENSIONS OFF
    WIN32_EXECUTABLE TRUE
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    EXPORT_NAME ${QWT_APP_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS ${QWT_APP_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
