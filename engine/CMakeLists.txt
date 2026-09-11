#[[====================================================
#
#  PROJECT: engine
#
=====================================================]]
cmake_minimum_required(VERSION 4.00)

#project name and version
project(gosEngine VERSION 1.0.0 LANGUAGES C CXX)

### info utili per iniziale con cmake
# nella cartella, crea CMakeLists.txt e poi esegui 
# cmake -S . -B ./build  per configurare ilprj
# cmake --build ./build  per buildare
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#c++223
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)


#====================================================
# piattaforma di compilazione
set(GOS_PLATFORM__NAME "linux")
if (MSVC)
    set(GOS_PLATFORM__NAME "windows")
endif()

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

message("COMPILE_DEFINITIONS is ${COMPILE_DEFINITIONS}")
message("CMAKE_CONFIGURATION_TYPES  is ${CMAKE_CONFIGURATION_TYPES}")
message("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
message("CMAKE_BINARY_DIR is ${CMAKE_BINARY_DIR}")
if(MSVC AND BUILD_IS_DEBUG EQUAL 0 AND BUILD_IS_RELEASE EQUAL 0)
    SET(var_temp 0)
    string(FIND ${CMAKE_BINARY_DIR} "Debug" var_temp)
    if (var_temp EQUAL -1)
        set(BUILD_IS_RELEASE 1)
        set(CMAKE_BUILD_TYPE "Release")
    else()
        set(BUILD_IS_DEBUG 1)
        set(CMAKE_BUILD_TYPE "Debug")
    endif()
endif()


if(BUILD_IS_DEBUG)
    set(GOS_PLATFORM__FILE_POSTFIX "_d")
elseif(BUILD_IS_RELEASE)
    set(GOS_PLATFORM__FILE_POSTFIX "")
    
    #la seguente dovrebbe abilitare le ottimizzazioni link time (LTO) su tutte le piattaforme
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

#====================================================
# opzioni di compilazione
if (MSVC)
    add_compile_options(/W3)
    add_compile_options(-wd26495)       #ignore warning C26495, struct memeber maybe unitialized

    add_compile_definitions(UNICODE)
    add_compile_definitions(_UNICODE)
    add_compile_definitions(GOS_COMPILER__MSVC)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall)
    add_compile_options(-Wno-format-truncation)
    add_compile_definitions(GOS_COMPILER__GCC)
endif()

if(BUILD_IS_DEBUG)
    add_compile_definitions(_DEBUG)
    if (MSVC)
    else()
        #add_compile_options(-Og)    #-Og should be the optimization level of choice for the standard edit-compile-debug cycle
        add_compile_options(-fno-exceptions)
    endif()
elseif(BUILD_IS_RELEASE)
    add_compile_definitions(_NDEBUG)

    #flag di ottimizzazione
    if (MSVC)
        add_compile_options(-O2)    #max optimization
        add_compile_options(-Ob2)   #exand inline: any suitabile
        add_compile_options(-Ot)    #enable intrinsic fn: yes
        add_compile_options(-GL)    #favor fast code
    else()
        #add_compile_options(-O3)           #max optimization
        add_compile_options(-Ofast)         #-Ofast enables all -O3 optimizations. It also enables optimizations that are not valid for all standard-compliant programs. It turns on -ffast-math..
        add_compile_options(-flto)          #This option runs the standard link-time optimizer
        add_compile_options(-march=native)
        add_compile_options(-fno-exceptions)

        add_compile_options(-Wno-maybe-uninitialized)
    endif()
else()
    message(FATAL_ERROR "BUILD_IS_DEBUG is not set and BUILD_IS_RELEASE is not set either")
endif()

#====================================================
# composeBINARY_OUTPUT_FILENAME
# fn di comodo che ritorna il nome dell'exe/lib in base ai vari parametri di piattaforma
function(composeBINARY_OUTPUT_FILENAME projectName out__var)
    set(${out__var} "${projectName}__${GOS_PLATFORM__NAME}${GOS_PLATFORM__FILE_POSTFIX}" PARENT_SCOPE)
endfunction()


#====================================================
# makeLibraryName
# fn di comodo che ritorna il nome delle librerie a seconda che siamo in platform linux o windows
set(ABS_PATH_TO_LIB_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/lib")
function(makeLibraryName varName libName)
    composeBINARY_OUTPUT_FILENAME(${libName} composedLibName)
    if(GOS_PLATFORM__NAME STREQUAL "linux")
        set(temp "${ABS_PATH_TO_LIB_FOLDER}/lib${composedLibName}.a")
    elseif(GOS_PLATFORM__NAME STREQUAL "windows")
        set(temp "${ABS_PATH_TO_LIB_FOLDER}/${composedLibName}.lib")
    endif()

    if(NOT EXISTS ${temp})
        set(${varName} "WARNING: Can't find ${temp}" PARENT_SCOPE)
    else()
        set(${varName} "${temp}" PARENT_SCOPE)
    endif()    
endfunction()


#====================================================
# sotto progetti
add_subdirectory(${PROJECT_SOURCE_DIR}/src/external/glfw-3.3.8)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gos)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosMath)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosDB)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosGeom)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosShape)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosInput)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosImage)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosAsset2)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosGPU)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosGameUtils)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/gosEngine)



add_subdirectory(${PROJECT_SOURCE_DIR}/src/test/testKernel)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/test/testVulkan)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/tools/assetMonitor)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/test/land20260814)



