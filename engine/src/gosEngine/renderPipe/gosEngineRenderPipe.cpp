#include "gosEngineRenderPipe.h"
#include "../gosEngine.h"

using namespace gos;
using namespace gos::engine;

//*******************************************
RenderPipe::RenderPipe()
{
	ctx.allocator = NULL;
	ctx.engine = NULL;
	ctx.frame_number = 0;
	ctx.handle_rt0.setInvalid();
	ctx.handle_zbuffer.setInvalid();
	ctx.handle_descrSet0.setInvalid();
	ctx.handle_descrPool.setInvalid();
	ctx.handle_ubo_scene.setInvalid();
}

//*******************************************
void RenderPipe::unsetup()
{
	if (NULL == ctx.allocator)
		return;

	u32 n = renderer_list.getNElem();
	for (u32 i=0; i<n; i++)
	{
		renderer_list[i]->on__detach(ctx);
		GOSDELETE(ctx.allocator, renderer_list[i]);
	}
	renderer_list.unsetup();


    texture_array.unsetup();

	engine->release(handle_pipeline);

    engine->gpu->deleteResource(ctx.handle_zbuffer);
    engine->gpu->deleteResource(ctx.handle_rt0);
	engine->gpu->deleteResource(ctx.handle_ubo_scene);
    //gpu->deleteResource(handle_samplers[0]);
    //gpu->deleteResource(handle_samplers[1]);
    engine->gpu->deleteResource(ctx.handle_descrSet0);
	engine->gpu->deleteResource(ctx.handle_descrSet1);
    engine->gpu->deleteResource(ctx.handle_descrPool);
	

	ctx.allocator = NULL;
    ctx.engine = NULL;
}

//*******************************************
bool RenderPipe::setup (gos::Allocator *allocatorIN, Engine *engineIN)
{
	//Carico la pipeline di default
	//Questa mi serve soltanto per tirare fuori il descriptor-set0. E' una pipeline fake
	if (!engineIN->pipeline_createFromAsset ("gosengine_PIPE3", &handle_pipeline, res::eLoadMode::asap))
	{
		DBGBREAK;
		return false;
	}

	engine = engineIN;
	gos::GPU *gpu = engineIN->gpu;
	ctx.allocator = allocatorIN;
	ctx.engine = engineIN;


	//rt0
	if (!gpu->renderTarget_create ("0-", "0-", eImageFormat::U8_RGBA, &ctx.handle_rt0))
	{
		gos::logger::err ("RenderPipe::setup() => gpu->renderTarget_create()\n");
		return false;
	}

	//zbuffer
	if (!gpu->zbuffer_create ("0-", "0-", eImageFormat::_DEPTH_BEST, &ctx.handle_zbuffer))
	{
		gos::logger::err ("RenderPipe::setup() => GPU::zbuffer_create()\n");
		return false;
	}

	//creo un descriptor pool
	gpu->descrPool_createNew (&ctx.handle_descrPool)
		.setMaxNumDescriptorSet(16)
		.addPool_uniformBuffer(8)
		.addPool_storageBuffer(8)
		.addPool_sampler(2)
		.addPool_texture(NUM_MAX_TEXTURE)
		.end();
	if (ctx.handle_descrPool.isInvalid())
	{
		gos::logger::err ("RenderPipe::setup() => can't create descriptor pool\n");
		return false;
	}

    //creo gli oggetti che poi dovro' bindare ai descrittori
    texture_array.setup(ctx.allocator, NUM_MAX_TEXTURE);
    {
        gpu::SamplerDesc desc;

        //sampler2d: bilinear filtering
        desc.reset();
        gpu->sampler_create (desc, &handle_samplers[0]);

        //sampler2d: point filtering
        desc.reset();
        desc.minFilter = desc.magFilter = eSamplerFilter::point;
        desc.mipFilter = eSamplerMipFilter::nearest;
        desc.bAnisotropic = false;
        gpu->sampler_create (desc, &handle_samplers[1]);
    }

    //UBO "scene"
    gpu->uniformBuffer_create (sizeof(SceneData), eMemAccessMode::shared_cpuW_autoSync, &ctx.handle_ubo_scene);


	//altre risorse
	renderer_list.setup (ctx.allocator, 16);



    //attendo che la pipe sia stata caricata perche' mi servono le definizioni dei descrittori
    const res::Pipeline *res_pipeline;
    if (!engine->get (handle_pipeline, &res_pipeline, 5000))
	{
		DBGBREAK;
		return false;
	}

	//alloco una istanza dei descriptor-set
	gos::gpu::DescrSetInstanceWriter dsw;

	//descriptor set 0
	if (!gpu->descrSetInstance_create (ctx.handle_descrPool, res_pipeline->pipeHandle, 0, &ctx.handle_descrSet0))
	{
		gos::logger::err ("RenderPipe::setup() => can't create an instance of descriptorSet_0\n");
		return false;
	}
	else
	{
		dsw.begin (gpu, ctx.handle_descrSet0)
			.bindSamplerInArray  (0, handle_samplers[0], 0)             //bindo in samplerList[0] il sampler "bilinear"
			.bindSamplerInArray  (0, handle_samplers[1], 1)             //bindo in samplerList[1] il sampler "point"
			.end();
	}

	//descriptor set 1
	if (!gpu->descrSetInstance_create (ctx.handle_descrPool, res_pipeline->pipeHandle, 1, &ctx.handle_descrSet1))
	{
		gos::logger::err ("RenderPipe::setup() => can't create an instance of descriptorSet_1\n");
		return false;
	}
	else
	{
		dsw.begin (gpu, ctx.handle_descrSet1)
			.bindUniformBuffer (0, ctx.handle_ubo_scene, 0)
			.end();
	}		

	//la pipe non mi serve +
	//engine->release(handle_pipeline);
	return true;
}


//*******************************************
u32	RenderPipe::texture_addIfNotExitst (GPUTextureHandle texHandle)
{ 
	assert (ctx.handle_descrSet0.isValid());

    bool bWasNew;
    const u32 texture_index = texture_array.addIfNotExitst(texHandle, &bWasNew);
    if (bWasNew)
    {
        gos::gpu::DescrSetInstanceWriter dsw;
        dsw.begin (engine->gpu, ctx.handle_descrSet0)
            .bindTextureInArray (1, texHandle, texture_index)
            .end();
    }

    return texture_index;
}

//*******************************************
void RenderPipe::priv_add_renderer (Renderer *r)
{ 
	renderer_list.append(r);
	r->on__attach(ctx); 
}

//*******************************************
void RenderPipe::remove_renderer (Renderer *r)
{ 
	const u32 n = renderer_list.getNElem();
	for (u32 i=0; i<n; i++)
	{
		if (renderer_list(i) == r)
		{
			r->on__detach(ctx); 
			GOSDELETE(ctx.allocator, renderer_list[i]);
			renderer_list.removeAndSwapWithLast(i);
			return;
		}
	}
}


//*******************************************
void RenderPipe::render (gos::gpu::SwapchainImg swapchainImg, GPUCmdBufferHandle cmdBufferHandle, gos::geom::Camera3 *cam)
{
    //aggiorno UBO descrittore scena
	ctx.scene.matVP = cam->getMatVP();
	ctx.scene.lightDir = vec4f (cam->pos.getAsseZ(), 0);
	ctx.scene.lightDir.set (-0.3f, -1.0f, 0.3f,    0.2f);
	//scene.lightDir.set (0, -1.0f, 0,    0);
	ctx.scene.lightDir.normalize();

	engine->gpu->writeAndSync (ctx.handle_ubo_scene, 0, &ctx.scene, sizeof(ctx.scene));


	//aggiorno il ctx
	ctx.frame_number++;
	ctx.cam = cam;

	//bakcground color
	gos::ColorHDR bgcol(0xFF6ec8d4);
	bgcol.sRGBToLinear();

	//cmd buffer
	gos::gpu::CmdBufferWriter2 cw;
	cw	.begin (engine->gpu, cmdBufferHandle)
		.setViewport (engine->gpu->viewport_getDefault())
		.imageTransition (ctx.handle_rt0, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
		.imageTransition (ctx.handle_zbuffer, eImageLayout::undefined, eImageLayout::depth_attachment_optimal);

	gpu::RenderCtx rctx;
	cw  .renderCtx_define_begin(&rctx)
			.withRenderArea (ctx.handle_rt0)
			.withRT (ctx.handle_rt0, eAttachmentLoadOp::clear, eAttachmentStoreOp::store, bgcol)
			.withZB (ctx.handle_zbuffer, eAttachmentLoadOp::clear, eAttachmentStoreOp::store)
		.define_end();

	const u32 n = renderer_list.getNElem();
	for (u32 i=0; i<n; i++)
	{
		renderer_list[i]->on__render (ctx, rctx);
	}
	
	rctx.end_render_ctx();


	
	

	cw	.imageTransition (ctx.handle_rt0, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
		.imageTransition (swapchainImg.image, eImageLayout::undefined, eImageLayout::transfer_dst)
		.copyImageToImage (ctx.handle_rt0, swapchainImg.image, engine->gpu->swapChain_getImageExten2D(), engine->gpu->swapChain_getImageExten2D())
		.imageTransition (swapchainImg.image, eImageLayout::transfer_dst, eImageLayout::presentation)
		.end();	
}

