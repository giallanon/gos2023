#[[====================================================
#
#  PROJECT: gosInput
#
=====================================================]]
cmake_minimum_required(VERSION 3.20)

#project name and version
project(gosInput VERSION 1.0.0 LANGUAGES C CXX)

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
gosInput.cpp
gosInput.h
gosInputEnumAndDefine.h
gosInputEvtList.cpp
gosInputEvtList.h
gosInputMapper.cpp
gosInputMapper.h
gosInputWindow.cpp
gosInputWindow.h
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
# GLFW-3.38 library
set(PATH_TO_GLFW "${PROJECT_SOURCE_DIR}/../external/glfw-3.3.8")
include_directories(${PATH_TO_GLFW}/include)