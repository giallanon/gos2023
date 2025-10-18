#include "test1.h"
#include "gosShapePrefabs.h"


using namespace gos;


//***************************************
Test1::Test1()
{
	allocator = gos::getSysHeapAllocator();

	entRegistry.setup();
	entRegistry.addComponentHandler<ent::CompPos>();
	entRegistry.addComponentHandler<ent::CompModelInstance>();

	entList.setup (gos::getSysHeapAllocator(), 32);

	skeleton = NULL;
	model = NULL;
	modelInstance = NULL;
}


//***************************************
Test1::~Test1()
{
	entList.unsetup();
	entRegistry.unsetup();

	GOSDELETE(allocator, modelInstance);
	GOSDELETE(allocator, model);
	GOSDELETE(allocator, skeleton);
}

//***************************************
void Test1::priv_model_setup(gos::ENGShape shapeHandle)
{
	skeleton = GOSNEW(allocator, gos::Skeleton)();
	{
		gos::SkeletonBuilder builder;

		gos::Bone *bone;
		const u32 iRoot = builder.begin ("root");
		builder.addChildTo (iRoot, "left-arm", &bone);
			bone->matrix.buildTranslation(vec3f(-4,0,0));
		builder.addChildTo (iRoot, "right-arm", &bone);
			bone->matrix.buildTranslation(vec3f( 4,0,0));
		builder.end (gos::getSysHeapAllocator(), skeleton);
	}

	model = GOSNEW(allocator, model::Model)();
	{
		model->setSkeleton(skeleton);
		model->addShape (shapeHandle);
		model->linkShapeToBone (shapeHandle, "root");
		model->linkShapeToBone (shapeHandle, "left-arm");
		model->linkShapeToBone (shapeHandle, "right-arm");
	}


	modelInstance = GOSNEW(allocator, model::ModelInstance)(model);
	
 }

//***************************************
void Test1::run (gos::Engine *engine)
{
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


		//creo un job per pushare lo stage buffer in VB/IB
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



	//model 	
	priv_model_setup(shapeHandle);

	for (u32 i=0; i<3; i++)
	{
		Entity ent = entRegistry.newEntity();
		{
			auto pos = entRegistry.addComponent<ent::CompPos>(ent);
			pos->matrix.buildTranslation ( vec3f(0, 0, (f32)(10 * i)) );

			auto mi = entRegistry.addComponent<ent::CompModelInstance>(ent);
			mi->modelInstance = this->modelInstance;
			mi->modelInstance->applyTransform (pos->matrix);
		}

		entList.append(ent);
	}



	//loop
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (engine->gpu);


    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    engine->gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);


	bool bQuit = false;
	while (false == bQuit)
	{
		if (!engine->update())
			bQuit = true;

//		gos::gpu::pipe2::CmdBufferWriter2 cw;
		
	}

	engine->gpu->deleteResource (cmdBufferHandle);
	engine->shape_release(shapeHandle);
}