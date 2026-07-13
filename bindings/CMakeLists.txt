# bindings/CMakeLists.txt
# PySide6 / Shiboken6 bindings for Qwt — orchestrator.
# Each module (core, plot, plot3d) has its own subdirectory with its own
# CMakeLists.txt, global.h, and typesystem XML.

cmake_minimum_required(VERSION 3.5)

# ---- Dependencies -----------------------------------------------------------
# Qt6 must already be found by the top-level CMakeLists.txt (find_package(QT NAMES Qt6 ...)).

find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Locate shiboken6_generator (pip-installed) and add its cmake dir to CMAKE_PREFIX_PATH.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(PySide6Shiboken REQUIRED)

# Now Shiboken6Tools is discoverable.
find_package(Shiboken6Tools REQUIRED)

add_subdirectory(core)
add_subdirectory(plot)
