#[[====================================================
#
#  PROJECT: gos
#
=====================================================]]
cmake_minimum_required(VERSION 3.20)

#project name and version
project(gos VERSION 1.0.0 LANGUAGES C CXX)

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

dataTypes/gosDate.cpp
dataTypes/gosDate.h
dataTypes/gosDateTime.cpp
dataTypes/gosDateTime.h
dataTypes/gosTime24.cpp
dataTypes/gosTime24.h
dataTypes/gosTimer.cpp
dataTypes/gosTimer.h

helpers/mtrand.cpp
helpers/mtrand.h

logger/gosLogger.h
logger/gosLoggerNull.h
logger/gosLoggerStdout.cpp
logger/gosLoggerStdout.h

memory/Doug_Lea_malloc.c 
memory/Doug_Lea_malloc.h
memory/gosAllocator.h
memory/gosAllocatorFixedHeap.h
memory/gosAllocatorHeap.h
memory/gosAllocatorPolicy_Thread.h
memory/gosAllocatorPolicy_Track.h
memory/gosMemory.h
memory/gosMemory.cpp

platform/gosPlatform.h 
platform/gosPlatformInclude.h 
platform/linux/linuxOS.cpp    
platform/linux/linuxOS.h
platform/linux/linuxOSFS.cpp
platform/linux/linuxOSConsole.cpp
platform/linux/linuxOSInclude.h

platform/win/winOS.cpp
platform/win/winOS.h
platform/win/winOSConsole.cpp
platform/win/winOSConsole.h
platform/win/winOSFS.cpp
platform/win/winOSInclude.h

string/gosString.cpp 
string/gosStringANSI.cpp 
string/gosStringEnumAndDefine.h
string/gosStringFormat.cpp
string/gosStringIter.cpp
string/gosUTF8.cpp 
string/gosUTF8Char.cpp 
string/gosUTF16.cpp
string/gosUTF32.cpp 

gos.cpp
gos.h
gosArray.h
gosBufferLinear.cpp 
gosBufferLinear.h
gosBufferSparse.h
gosBufferSparse.cpp
gosConsole.cpp
gosDataTypes.h
gosEnumAndDefine.h
gosFastArray.h
gosFIFO.h
gosFIFOFixedSize.h
gosFS.cpp
gosHandle.h
gosLIFOFixedSize.h
gosRandom.h
gosString.h 
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set_source_files_properties(memory/Doug_Lea_malloc.c PROPERTIES COMPILE_OPTIONS "-Wno-expansion-to-defined;-Wno-maybe-uninitialized")
	set_source_files_properties(memory/Doug_Lea_malloc.h PROPERTIES COMPILE_OPTIONS "-Wno-expansion-to-defined;-Wno-maybe-uninitialized")
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