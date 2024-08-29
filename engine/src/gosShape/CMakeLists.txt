#[[====================================================
#
#  PROJECT: gosShape
#
=====================================================]]
cmake_minimum_required(VERSION 3.20)

#project name and version
project(gosShape VERSION 1.0.0 LANGUAGES C CXX)

#====================================================
# path di output di questo progetto
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../lib)


#====================================================
# nome del file di output
set(BINARY_OUTPUT_FILENAME "${PROJECT_NAME}__${GOS_PLATFORM__NAME}_${CMAKE_BUILD_TYPE}")

#====================================================
# additional include path
include_directories(${PROJECT_SOURCE_DIR}/../../gos)


#====================================================
# file da compilare
add_library(${BINARY_OUTPUT_FILENAME} STATIC
gosShape.h
gosShapeColladaImporter.cpp
gosShapeColladaImporter.h
gosShapeEnumAndDefine.h
gosShapePrefabs_cube.cpp
gosShapePrefabs_cylinder.cpp
gosShapePrefabs_sphere.cpp
gosShapePrefabs.h
gosShapeVtxLayout.cpp

../external/tinyxml/tinyxml2.cpp
../external/tinyxml/tinyxml2.h
)

#====================================================
# info a video
message(" ")
message(" ")
message("GIXINFO: prj name = ${PROJECT_NAME} v${PROJECT_VERSION}")
message("         prj src dir = ${PROJECT_SOURCE_DIR}")
message("         current config = ${GOS_PLATFORM__NAME} / ${CMAKE_BUILD_TYPE}")
message("         ouput file = ${BINARY_OUTPUT_FILENAME}")
get_target_property(MAIN_CFLAGS ${BINARY_OUTPUT_FILENAME} COMPILE_OPTIONS)
message("         compiler flags: ${MAIN_CFLAGS}")