﻿cmake_minimum_required(VERSION 3.5)
project(QwtPlayground LANGUAGES CXX)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../classincludes)
add_subdirectory(curvetracker)
add_subdirectory(graphicscale)
add_subdirectory(plotmatrix)
add_subdirectory(rescaler)
add_subdirectory(scaleengine)
add_subdirectory(shapes)
add_subdirectory(svgmap)
add_subdirectory(symbols)
add_subdirectory(timescale)
add_subdirectory(vectorfield)
