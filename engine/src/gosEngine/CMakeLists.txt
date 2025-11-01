#[[====================================================
#
#  PROJECT: gosEngine
=====================================================]]
cmake_minimum_required(VERSION 4.00)

#project name and version
project(gosEngine VERSION 1.0.0 LANGUAGES C CXX)

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
entity/gosEntityComponentList.h
entity/gosEntityEnumAndDefine.h
entity/gosEntityRegistry.cpp
entity/gosEntityRegistry.h

model/gosModel.cpp
model/gosModel.h
model/gosSkeleton.cpp
model/gosSkeleton.h

gosEngine_dynTextureArray.cpp
gosEngine_dynTextureArray.h
gosEngine_fixedSizeBufferTracker.cpp
gosEngine_fixedSizeBufferTracker.h
gosEngine_idxBufferMan.cpp
gosEngine_idxBufferMan.h
gosEngine_renderer.cpp
gosEngine_renderer.h
gosEngine_vtxBufferMan.cpp
gosEngine_vtxBufferMan.h
gosEngine.cpp
gosEngine.h
gosEngineEnumAndDefine.h
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

#====================================================
# additional include path
# GLFW-3.38 library
set(PATH_TO_GLFW "${PROJECT_SOURCE_DIR}/../external/glfw-3.3.8")
include_directories(${PATH_TO_GLFW}/include)

#===================================
# vulkan library
find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan development package not found")
else()
    message("         vulkan version ${Vulkan_VERSION}")
    message("         vulkan include path ${Vulkan_INCLUDE_DIRS}")
endif()
#target_include_directories(${BINARY_OUTPUT_FILENAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
#target_link_libraries(${BINARY_OUTPUT_FILENAME} "${Vulkan_LIBRARIES}")
include_directories(${Vulkan_INCLUDE_DIRS})

