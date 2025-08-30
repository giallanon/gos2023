#[[====================================================
#
#  PROJECT: glslParser
#
=====================================================]]
cmake_minimum_required(VERSION 4.00)

#project name and version
project(glslParser VERSION 1.0.0 LANGUAGES C CXX)

#====================================================
# path di output di questo progetto
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../../bin/${PROJECT_NAME})

#====================================================
# nome del file di output
set(BINARY_OUTPUT_FILENAME "${PROJECT_NAME}__${GOS_PLATFORM__NAME}${GOS_PLATFORM__FILE_POSTFIX}")


#====================================================
#nome del file generato, elenco dei src
add_executable(${BINARY_OUTPUT_FILENAME}
main.cpp
PipelineDef.cpp
PipelineParser.cpp
PipelineParser.h
spirv_reflect.c
spirv_reflect.h
spirv.h
SPVReflect.cpp
SPVReflect.h
)

#====================================================
# additional include path
include_directories(${PROJECT_SOURCE_DIR}/../../gos)
include_directories(${PROJECT_SOURCE_DIR}/../../gosDB)
include_directories(${PROJECT_SOURCE_DIR}/../../gosAsset)



#====================================================
# additional library to be linked
set(LIBRARY_LIST "gosAsset;gosDB;gos")
foreach(libName IN LISTS LIBRARY_LIST)
    makeLibraryName(finalLibNameAndPath ${libName})
    message("         linkling libray [${libName}] => ${finalLibNameAndPath}")
    target_link_libraries(${BINARY_OUTPUT_FILENAME} ${finalLibNameAndPath})
endforeach()


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



