#[[====================================================
#
#  PROJECT: testEngine
#
=====================================================]]
cmake_minimum_required(VERSION 4.00)

#project name and version
project(testEngine VERSION 1.0.0 LANGUAGES C CXX)

#====================================================
# path di output di questo progetto
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../../bin/${PROJECT_NAME})

#====================================================
# nome del file di output
set(BINARY_OUTPUT_FILENAME "${PROJECT_NAME}__${GOS_PLATFORM__NAME}${GOS_PLATFORM__FILE_POSTFIX}")


#====================================================
#nome del file generato, elenco dei src
add_executable(${BINARY_OUTPUT_FILENAME}
DefaultApp/DefaultApp.cpp
DefaultApp/DefaultApp.h

Land1/Land1_app.cpp
Land1/Land1_app.h
Land1/Land1_enumAndDefine.h
Land1/Land1_Exa.cpp
Land1/Land1_exaGenerator.cpp
Land1/Land1_exaGenerator.h
Land1/Land1_map.cpp
Land1/Land1_map.h
Land1/Land1_renderer.cpp
Land1/Land1_renderer.h


characterController.cpp
characterController.h
ExaGenerator.cpp
game1.cpp
game1.h
main.cpp
PerlinNoise.hpp
test_exa1.cpp
test_exa1.h
test1.cpp
test1.h
 )


 #====================================================
# info a video
message(" ")
message(" ")
message("GIXINFO: prj name = ${PROJECT_NAME} v${PROJECT_VERSION}")
message("         prj src dir = ${PROJECT_SOURCE_DIR}")
message("         current config = ${CMAKE_BUILD_TYPE}")
message("         ouput file = ${BINARY_OUTPUT_FILENAME}")
get_target_property(MAIN_CFLAGS ${BINARY_OUTPUT_FILENAME} COMPILE_OPTIONS)
message("         compiler flags: ${MAIN_CFLAGS}")


#====================================================
# additional include path
include_directories(${PROJECT_SOURCE_DIR}/../../gos)
include_directories(${PROJECT_SOURCE_DIR}/../../gosMath)
include_directories(${PROJECT_SOURCE_DIR}/../../gosGeom)
include_directories(${PROJECT_SOURCE_DIR}/../../gosShape)
include_directories(${PROJECT_SOURCE_DIR}/../../gosInput)
include_directories(${PROJECT_SOURCE_DIR}/../../gosImage)
include_directories(${PROJECT_SOURCE_DIR}/../../gosGPU)
include_directories(${PROJECT_SOURCE_DIR}/../../gosAsset2)
include_directories(${PROJECT_SOURCE_DIR}/../../gosDB.h)
include_directories(${PROJECT_SOURCE_DIR}/../../gosEngine)

#====================================================
# additional library to be linked
set(LIBRARY_LIST "gosGameUtils;gosEngine;gosAsset2;gosGPU;gosDB;gosImage;gosInput;gosShape;gosGeom;gosMath;gos")
foreach(libName IN LISTS LIBRARY_LIST)
    makeLibraryName(finalLibNameAndPath ${libName})
    message("         linkling additional libray [${libName}] => ${finalLibNameAndPath}")
    target_link_libraries(${BINARY_OUTPUT_FILENAME} ${finalLibNameAndPath})
endforeach()


#===================================
# GLFW-3.38 library
set(PATH_TO_GLFW "${PROJECT_SOURCE_DIR}/../../external/glfw-3.3.8")
include_directories(${PATH_TO_GLFW}/include)
makeLibraryName(glfwLibraryFilename glfw3)
target_link_libraries(${BINARY_OUTPUT_FILENAME} "${glfwLibraryFilename}")


#===================================
# vulkan library
find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan development package not found")
endif()
target_include_directories(${BINARY_OUTPUT_FILENAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${BINARY_OUTPUT_FILENAME} "${Vulkan_LIBRARIES}")
include_directories(${Vulkan_INCLUDE_DIRS})
message("         vulkan version: ${Vulkan_VERSION}")
message("         vulkan found, path is ${Vulkan_LIBRARIES}")
message("         vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
message("         vulkan SPIRV tools found: ${Vulkan_SPIRV-Tools_FOUND}")

#===================================
# x11 library
if(GOS_PLATFORM__NAME STREQUAL "linux")
    find_package(X11 REQUIRED)
    if (NOT X11_FOUND)
        message(FATAL_ERROR "X11 development package not found")
    endif()
    target_link_libraries(${BINARY_OUTPUT_FILENAME} "${X11_LIBRARIES}")
endif()
