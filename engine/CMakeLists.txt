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
    set(GOS_PLATFORM__FILE_POSTFIX "_d")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(BUILD_IS_RELEASE 1)
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
endif()


#====================================================
# makeLibraryName
# fn di comodo che ritorna il nome delle librerie a seconda che siamo in platform linux o windows
set(ABS_PATH_TO_LIB_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/lib")
function(makeLibraryName varName libName)
    if(GOS_PLATFORM__NAME STREQUAL "linux")
        set(temp "${ABS_PATH_TO_LIB_FOLDER}/lib${libName}__${GOS_PLATFORM__NAME}${GOS_PLATFORM__FILE_POSTFIX}.a")
    elseif(GOS_PLATFORM__NAME STREQUAL "windows")
        set(temp "${ABS_PATH_TO_LIB_FOLDER}/${libName}__${GOS_PLATFORM__NAME}${GOS_PLATFORM__FILE_POSTFIX}.lib")
    endif()

    set(${varName} "${temp}" PARENT_SCOPE)
    if(NOT EXISTS ${temp})
        message(WARNING "Can't find ${temp}")
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



