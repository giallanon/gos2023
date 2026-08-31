#[[====================================================
#
#  PROJECT: gosGPU
=====================================================]]
cmake_minimum_required(VERSION 4.00)

#project name and version
project(gosGPU VERSION 1.0.0 LANGUAGES C CXX)

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
pipe2/gosGPUPipe2_cmdBufferWriter_RenderCtx.cpp
pipe2/gosGPUPipe2_cmdBufferWriter_RenderCtxProps.cpp
pipe2/gosGPUPipe2_cmdBufferWriter_SetupRenderCtx.cpp
pipe2/gosGPUPipe2_cmdBufferWriter.cpp
pipe2/gosGPUPipe2_cmdBufferWriter.h
pipe2/gosGPUPipe2_pipeline_def.h

vulkan/gosGPUVukanHelpers.cpp
vulkan/gosGPUVukanHelpers.h
vulkan/gosGPUVulkan.cpp
vulkan/gosGPUVulkan.h
vulkan/gosGPUVulkanDevice.cpp
vulkan/gosGPUVulkanDevice.h
vulkan/gosGPUVulkanEnumAndDefine.h
vulkan/gosGPUVulkanFeatures.cpp
vulkan/gosGPUVulkanQFamily.cpp
vulkan/gosGPUVulkanQFamily.h

utils/gosGPUMainLoop.cpp
utils/gosGPUMainLoop.h


gosGPU_DescriptorPoolBuilder.cpp
gosGPU_ImmediateTransferCmd.cpp
gosGPU.cpp
gosGPU.h
gosGPUDescrSetInstanceWriter.cpp
gosGPUDescrSetInstanceWriter.h
gosGPUEnumAndDefine.h
gosGPUMappedBuffer.cpp
gosGPUMappedBuffer.h
gosGPUResBuffer.h
gosGPUResCommandBuffer.h
gosGPUResDepthStencil.h
gosGPUResDescrPool.h
gosGPUResDescrSetInstance.h
gosGPUResDescrSetLayout.h
gosGPUResRenderTarget.h
gosGPUResSampler.h
gosGPUResShader.h
gosGPUResTexture.h
gosGPUResViewport.h

GOSGPUStageHelper.cpp
GOSGPUStageHelper.h
gosGPUUtils.cpp
gosGPUUtils.h
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

