cmake_minimum_required(VERSION 3.5)
SET(VERSION_SHORT 0.1)
SET(QWT_APP_NAME staticExample)
project(${QWT_APP_NAME} VERSION ${VERSION_SHORT})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# qt库加载，最低要求5.8
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.8 COMPONENTS Core Gui Widgets Svg Concurrent OpenGL PrintSupport REQUIRED)
add_executable(${QWT_APP_NAME} 
    main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../src-amalgamate/QwtPlot.h
    ${CMAKE_CURRENT_LIST_DIR}/../../src-amalgamate/QwtPlot.cpp
)

target_include_directories(${QWT_APP_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../../src-amalgamate")
target_link_libraries(${QWT_APP_NAME} PUBLIC
                                       Qt${QT_VERSION_MAJOR}::Core 
                                       Qt${QT_VERSION_MAJOR}::Gui 
                                       Qt${QT_VERSION_MAJOR}::Widgets
                                       Qt${QT_VERSION_MAJOR}::Svg
                                       Qt${QT_VERSION_MAJOR}::Concurrent
                                       Qt${QT_VERSION_MAJOR}::OpenGL
                                       Qt${QT_VERSION_MAJOR}::PrintSupport)


set_target_properties(${QWT_APP_NAME} PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    WIN32_EXECUTABLE TRUE
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    EXPORT_NAME ${QWT_APP_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS ${QWT_APP_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)
