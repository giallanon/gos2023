#[[====================================================
#
#  PROJECT: testVulkan
#
=====================================================]]
cmake_minimum_required(VERSION 3.20)

#project name and version
project(testVulkan VERSION 1.0.0 LANGUAGES C CXX)

#====================================================
# path di output di questo progetto
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../../bin)

#====================================================
# nome del file di output
set(BINARY_OUTPUT_FILENAME "${PROJECT_NAME}__${GOS_PLATFORM__NAME}_${CMAKE_BUILD_TYPE}")



#LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

#====================================================
#nome del file generato, elenco dei src
add_executable(${BINARY_OUTPUT_FILENAME}
    main.cpp
    VKExample1.cpp
    VKExample1.h
)

#====================================================
# additional include path
include_directories(${PROJECT_SOURCE_DIR}/../../gos)
include_directories(${PROJECT_SOURCE_DIR}/../../gosMath)
include_directories(${PROJECT_SOURCE_DIR}/../../gosGeom)

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
# additional library to be linked
set(LIBRARY_LIST "gos;gosMath;gosGeom")
foreach(libName IN LISTS LIBRARY_LIST)
    makeLibraryName(finalLibNameAndPath ${libName})
    #set(finalLibNameAndPath "${PROJECT_SOURCE_DIR}/../../../lib/lib${libName}__${GOS_PLATFORM__NAME}_${CMAKE_BUILD_TYPE}.a")
    message("         linkling libray [${libName}] => ${finalLibNameAndPath}")
    target_link_libraries(${BINARY_OUTPUT_FILENAME} ${finalLibNameAndPath})
endforeach()

#===================================
# GLFW-3.38 library
set(PATH_TO_GLFW "${PROJECT_SOURCE_DIR}/../../external/glfw-3.3.8")
include_directories(${PATH_TO_GLFW}/include)
if(GOS_PLATFORM__NAME STREQUAL "linux")
    target_link_libraries(${BINARY_OUTPUT_FILENAME} "${PATH_TO_GLFW}/build/src/libglfw3.a")
elseif(GOS_PLATFORM__NAME STREQUAL "windows")
endif()

#===================================
# vulkan library
find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan development package not found")
endif()
target_link_libraries(${BINARY_OUTPUT_FILENAME} "${Vulkan_LIBRARIES}")

#===================================
# x11 library
if(GOS_PLATFORM__NAME STREQUAL "linux")
    find_package(X11 REQUIRED)
    if (NOT X11_FOUND)
        message(FATAL_ERROR "X11 development package not found")
    endif()
    target_link_libraries(${BINARY_OUTPUT_FILENAME} "${X11_LIBRARIES}")
endif()