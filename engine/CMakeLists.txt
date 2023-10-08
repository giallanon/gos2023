#[[====================================================
#
#  PROJECT: engine
#
=====================================================]]
cmake_minimum_required(VERSION 3.27.5)

#project name and version
project(gosEngine VERSION 1.0.0 LANGUAGES C CXX)

### info utili per iniziale con cmake
# nella cartella, crea CMakeLists.txt e poi esegui 
# cmake -S . -B ./build  per configurare ilprj
# cmake --build ./build  per buildare
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#c++14
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


#====================================================
# piattaforma di compilazione
set(GOS_PLATFORM__NAME "linux")

if(GOS_PLATFORM__NAME STREQUAL "linux")
    add_compile_definitions(GOS_PLATFORM__LINUX)
elseif(GOS_PLATFORM__NAME STREQUAL "windows")
    add_compile_definitions(GOS_PLATFORM__WINDOWS)
endif()


#====================================================
# determina quale build stiamo compilando
set(BUILD_IS_DEBUG 0)
set(BUILD_IS_RELEASE 0)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BUILD_IS_DEBUG 1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(BUILD_IS_RELEASE 1)
endif()


#====================================================
# opzioni di compilazione
if (MSVC)
    add_compile_options(/W4)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wpedantic)
endif()

if(BUILD_IS_DEBUG)
    add_compile_definitions(_DEBUG)
elseif(BUILD_IS_RELEASE)
    add_compile_definitions(_NDEBUG)
    add_compile_options(-O3)
endif()

#====================================================
# sotto progetti
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gos)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosMath)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosGeom)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/test/testKernel)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/test/testVulkan)