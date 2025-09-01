#[[====================================================
#
#  PROJECT: gosDB
#
=====================================================]]
cmake_minimum_required(VERSION 4.00)

#project name and version
project(gosDB VERSION 1.0.0 LANGUAGES C CXX)

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
sqlite-3.50.4/sqlite3.c
sqlite-3.50.4/sqlite3.h
gosDB.cpp
gosDB.h
gosRST.cpp
gosRST.h
)

#sqlite3.c tira un sacco di warning per cui aggiungo queste opzioni di compilazione per non vederli
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_source_files_properties (sqlite-3.50.4/sqlite3.c COMPILE_FLAGS "-Wno-stringop-overread")
endif()
                                                                    

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

