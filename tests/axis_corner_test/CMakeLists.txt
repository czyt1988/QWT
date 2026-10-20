cmake_minimum_required(VERSION 3.10)
SET(VERSION_SHORT 0.1)
SET(QWT_TEST_NAME axis_corner_test)
project(${QWT_TEST_NAME} VERSION ${VERSION_SHORT})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt库加载，需要Test模块
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.8 COMPONENTS Core Gui Widgets Test REQUIRED)

file(GLOB TEST_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB TEST_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${QWT_TEST_NAME} 
    ${TEST_HEADER_FILES}
    ${TEST_SOURCE_FILES}
)

if(NOT TARGET qwt::plot)
    # 说明这个测试是单独加载
    message(STATUS "NOT TARGET qwt::plot find_package(qwt REQUIRED)")
    find_package(qwt REQUIRED)
endif()

target_link_libraries(${QWT_TEST_NAME} PUBLIC 
    qwt::plot
    Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(${QWT_TEST_NAME} PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    WIN32_EXECUTABLE FALSE
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    EXPORT_NAME ${QWT_TEST_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Add test to CTest
enable_testing()
add_test(NAME ${QWT_TEST_NAME} COMMAND ${QWT_TEST_NAME})

# The corner geometry depends on the device pixel ratio, and QT_SCALE_FACTOR
# must be set before QApplication is constructed, so every ratio runs in its
# own process (see main() in test_axis_corner.cpp).
set_tests_properties(${QWT_TEST_NAME} PROPERTIES
    ENVIRONMENT "QWT_TEST_SCALE_FACTOR=1.0")

add_test(NAME ${QWT_TEST_NAME}_dpr2_0 COMMAND ${QWT_TEST_NAME})
set_tests_properties(${QWT_TEST_NAME}_dpr2_0 PROPERTIES
    ENVIRONMENT "QWT_TEST_SCALE_FACTOR=2.0")

install(TARGETS ${QWT_TEST_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)
