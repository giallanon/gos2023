#[[====================================================
#
#  PROJECT: gos
#
=====================================================]]
cmake_minimum_required(VERSION 4.00)

#project name and version
project(gos VERSION 1.0.0 LANGUAGES C CXX)

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

dataTypes/gosColorHDR.cpp
dataTypes/gosColorHDR.h
dataTypes/gosColorU32.cpp
dataTypes/gosColorU32.h
dataTypes/gosDate.cpp
dataTypes/gosDate.h
dataTypes/gosDateTime.cpp
dataTypes/gosDateTime.h
dataTypes/gosPoint2.h
dataTypes/gosPosDim2D.cpp
dataTypes/gosPosDim2D.h
dataTypes/gosTime24.cpp
dataTypes/gosTime24.h
dataTypes/gosTimer.cpp
dataTypes/gosTimer.h

helpers/base64.cpp
helpers/debugbreak.h
helpers/mtrand.cpp
helpers/mtrand.h
helpers/sha1.cpp

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
platform/linux/linuxOSEvent.cpp
platform/linux/linuxOSFS.cpp
platform/linux/linuxOSConsole.cpp
platform/linux/linuxOSInclude.h
platform/linux/linuxOSSocket.cpp
platform/linux/linuxOSThread.cpp
platform/linux/linuxOSWaitableGrp.cpp
platform/linux/linuxOSWaitableGrp.h

platform/win/winOS.cpp
platform/win/winOS.h
platform/win/winOSConsole.cpp
platform/win/winOSConsole.h
platform/win/winOSFS.cpp
platform/win/winOSInclude.h
platform/win/winOSSocket.cpp
platform/win/winOSThread.cpp
platform/win/winOSWaitableGrp.cpp
platform/win/winOSWaitableGrp.h

protocol/gosIProtocol.cpp
protocol/gosIProtocol.h
protocol/gosIProtocolChannel.cpp
protocol/gosIProtocolChannel.h
protocol/gosProtocolBuffer.cpp
protocol/gosProtocolBuffer.h
protocol/gosProtocolChSocketTCP.cpp
protocol/gosProtocolChSocketTCP.h
protocol/gosProtocolConsole.cpp
protocol/gosProtocolConsole.h
protocol/gosProtocolEnumAndDefine.h
protocol/gosProtocolWebsocket.cpp
protocol/gosProtocolWebsocket.h

string/gosCompileTimeHashedString.h
string/gosString.cpp 
string/gosStringANSI.cpp 
string/gosStringEnumAndDefine.h
string/gosStringFormat.cpp
string/gosStringIncludeDetector.cpp
string/gosStringIncludeDetector.h
string/gosStringIter.cpp
string/gosStringList.cpp
string/gosStringList.h
string/gosUniqueStringList.h
string/gosUTF8.cpp 
string/gosUTF8Char.cpp 
string/gosUTF8String.cpp
string/gosUTF8String.h
string/gosUTF16.cpp
string/gosUTF32.cpp 

gos.cpp
gos.h
gosAllocTracker.cpp
gosAllocTracker.h
gosArray.h
gosBit.cpp
gosBit.h
gosBufferLinear.cpp 
gosBufferLinear.h
gosBufferReader.cpp
gosBufferReader.h
gosBufferSparse.h
gosBufferSparse.cpp
gosBufferWriter.cpp
gosBufferWriter.h
gosConsole.cpp
gosDataBlob_DefBuilder.cpp
gosDataBlob_DefReader.cpp
gosDataBlob.cpp
gosDataBlob.h
gosDataFormat.cpp
gosDataFormat.h
gosDataTypes.h
gosEnumAndDefine.h
gosErr.cpp
gosErr.h
gosFastArray.h
gosFIFO.h
gosFIFOFixedSize.h
gosFS_FSSpecialPathResolver.cpp
gosFS_FSSpecialPathResolver.h
gosFS.cpp
gosHandle.h
gosHashMap.h
gosIniFile_section.cpp
gosIniFile.cpp
gosIniFile.h
gosLIFOFixedSize.h
gosMagicUID.h
gosRandom.h
gosServerTCP.cpp
gosServerTCP.h
gosString.h
gosThreadMsgQ.h
gosThreadMsgQ.cpp
gosUtils.cpp
gosUtils.h
gosWaitableGrpInterface.h
gosWaitableGrp.h
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