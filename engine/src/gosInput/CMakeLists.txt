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
gosInputContext.cpp
gosInputContext.h
gosInputEnumAndDefine.h
gosInputEvtList.cpp
gosInputEvtList.h
gosInputModule.cpp
gosInputModule.h
gosInputResolvedEvtList.cpp
gosInputResolvedEvtList.h
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

#===================================
# vulkan library (richiesto da glFW)
find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan development package not found")
else()
    message("         vulkan include path ${Vulkan_INCLUDE_DIRS}")
endif()
#target_include_directories(${BINARY_OUTPUT_FILENAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
#target_link_libraries(${BINARY_OUTPUT_FILENAME} "${Vulkan_LIBRARIES}")
include_directories(${Vulkan_INCLUDE_DIRS})