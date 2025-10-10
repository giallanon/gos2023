#include "test1.h"
#include "gosShapePrefabs.h"

using namespace gos;


//***************************************
void Test1::run (gos::Engine *engine)
{
	gos::Allocator *allocator = gos::getSysHeapAllocator();

	//creo una shape
	gos::Shape shape;
	shape.reset();
	{
		gos::VtxLayout vtxLayout;
		{
			shape::VtxLayoutWriter vtxLayoutW;

			vtxLayoutW.setup (&vtxLayout);
			vtxLayoutW.begin()
				.addPos3(0)
				.addNorm3(12)
				.addTexCoord(24)
				.end();
		}

		if (!shape::buildCube24 (vec3f(0, 0, 0), vec3f(1, 1, 1), vtxLayout, allocator, &shape))
			return;
	}

	//creo una engine::shape
	gos::ENGShape shapeHandle;
	if (!engine->shape_create (&shape, &shapeHandle))
		return;

	//ora devo copiare vtx/idx nel VB/IB
	{
		//copio idx/vtx nello stage buffer
		GPUStgBufferHandle stgBufferHandle;
		engine->gpu->stagingBuffer_create (1024, &stgBufferHandle);
		
		const u32 SIZE_OF_IDX = shape.numIdx * sizeof(u16);
		engine->gpu->stagingBuffer_memcpy (stgBufferHandle, 0, shape.idxBuffer, SIZE_OF_IDX);

		const u32 SIZE_OF_VTX = shape::calcSizeOfAVertex(shape.vtxLayout) * shape.numVtx;
		engine->gpu->stagingBuffer_memcpy (stgBufferHandle, SIZE_OF_IDX, shape.vtxBuffer, SIZE_OF_VTX);

		//free della shape
		shape::shapeFree (allocator, &shape);


		//creo un job per pushare lo stage buffer in VB(IB
		GPUCmdBufferHandle cmdBufferHandle;
		if (!engine->gpu->cmdBuffer_create (eGPUQueueType::transfer, &cmdBufferHandle))
			return;
		const engine::Shape *shapeInfo = engine->shape_getInfo (shapeHandle);

		gos::gpu::pipe2::CmdBufferWriter2 cw;
		cw.begin (engine->gpu, cmdBufferHandle)
			.copyBuffer (stgBufferHandle, shapeInfo->ibHandle, 0, 0, SIZE_OF_IDX)
			.copyBuffer (stgBufferHandle, shapeInfo->vbHandle, SIZE_OF_IDX, 0, SIZE_OF_VTX)
			.end();


		gpu::TransferJob job;
		job.setup (engine->gpu);
		job.submit(cmdBufferHandle);

		while (!job.hasFinished())
		{
		}

		engine->gpu->deleteResource(cmdBufferHandle);
		engine->gpu->deleteResource(stgBufferHandle);

	}



	//loop
	bool bQuit = false;
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (engine->gpu);


    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    engine->gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);


	while (false == bQuit)
	{
		bQuit = engine->update();
	}


	engine->shape_release(shapeHandle);
}