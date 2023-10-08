#[[====================================================
#
#  PROJECT: gosMath
#
=====================================================]]
cmake_minimum_required(VERSION 3.27.5)

#project name and version
project(gosMath VERSION 1.0.0 LANGUAGES C CXX)

#====================================================
# path di output di questo progetto
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../lib)


#====================================================
# nome del file di output
set(BINARY_OUTPUT_FILENAME "${PROJECT_NAME}__${GOS_PLATFORM__NAME}_${CMAKE_BUILD_TYPE}")

#====================================================
# file da compilare
add_library(${BINARY_OUTPUT_FILENAME} STATIC
gosMath.cpp
gosMath.h
gosMathEnumAndDefine.h
gosMathMatrix3x3Helper.h
gosMatrix.h
gosQuat.cpp
gosQuat.h
gosVect.h
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