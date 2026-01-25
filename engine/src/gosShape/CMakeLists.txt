#[[====================================================
#
#  PROJECT: gosShape
#
=====================================================]]
cmake_minimum_required(VERSION 4.00)

#project name and version
project(gosShape VERSION 1.0.0 LANGUAGES C CXX)

#====================================================
# path di output di questo progetto
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../lib)


#====================================================
# nome del file di output
set(BINARY_OUTPUT_FILENAME "${PROJECT_NAME}__${GOS_PLATFORM__NAME}${GOS_PLATFORM__FILE_POSTFIX}")


#====================================================
# file da compilare
add_library(${BINARY_OUTPUT_FILENAME} STATIC
skeleton/gosSkeleton.cpp
skeleton/gosSkeleton.h
skeleton/gosSkeletonBuilder.cpp
skeleton/gosSkeletonInstance.cpp

gosShape.cpp
gosShape.h
gosShapeEnumAndDefine.h
gosShapeImport_arrays.cpp
gosShapeImport_arrays.h
gosShapeImport_glTF.cpp
gosShapeImport_glTF.h
gosShapeImport.cpp
gosShapeImport.h
gosShapePrefabs_cube.cpp
gosShapePrefabs_cylinder.cpp
gosShapePrefabs_sphere.cpp
gosShapePrefabs.h
gosShapeVtxArrayReader.cpp
gosShapeVtxArrayReader.h
gosShapeVtxArrayWriter.cpp
gosShapeVtxArrayWriter.h
gosShapeVtxElem.cpp
gosShapeVtxElem.h
gosShapeVtxLayout.h
gosShapeVtxLayout.cpp

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

#====================================================
# additional include path
include_directories(${PROJECT_SOURCE_DIR}/../gos)

