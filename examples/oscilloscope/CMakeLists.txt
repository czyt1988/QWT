cmake_minimum_required(VERSION 3.5)
SET(VERSION_SHORT 0.1)
SET(QWT_APP_NAME oscilloscope)
project(${QWT_APP_NAME} VERSION ${VERSION_SHORT})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# qt库加载，最低要求5.8
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.8 COMPONENTS Core Gui Widgets REQUIRED)
file(GLOB APP_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB APP_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(${QWT_APP_NAME} 
    ${APP_HEADER_FILES}
    ${APP_SOURCE_FILES}
)

if(NOT TARGET qwt)
    # 说明这个例子是单独加载
    message(STATUS "NOT TARGET qwt find_package(qwt REQUIRED)")
    find_package(qwt REQUIRED)
endif()

target_link_libraries(${QWT_APP_NAME} PUBLIC qwt::qwt)
target_link_libraries(${QWT_APP_NAME} PUBLIC
                                       Qt${QT_VERSION_MAJOR}::Core 
                                       Qt${QT_VERSION_MAJOR}::Gui 
                                       Qt${QT_VERSION_MAJOR}::Widgets)

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
