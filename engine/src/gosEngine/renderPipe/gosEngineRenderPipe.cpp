#include "gosEngineRenderPipe.h"
#include "../gosEngine.h"
#include <algorithm>

using namespace gos;
using namespace gos::engine;

//*******************************************
RenderPipe::RenderPipe()
{
	next_renderer_UID = 0;

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
void RenderPipe::priv_unsetup()
{
	if (NULL == ctx.allocator)
		return;

	//release dei renderer addizionali
	u32 n = renderer_list.getNElem();
	for (u32 i=0; i<n; i++)
	{
		renderer_list[i]->on__detach(ctx);
		GOSDELETE(ctx.allocator, renderer_list[i]);
	}
	renderer_list.unsetup();

	texture_array.unsetup();

	//release risorse di ctx
    engine->gpu->deleteResource(ctx.handle_zbuffer);
    engine->gpu->deleteResource(ctx.handle_rt0);
	engine->gpu->deleteResource(ctx.handle_ubo_scene);
    //gpu->deleteResource(handle_samplers[0]);
    //gpu->deleteResource(handle_samplers[1]);
    engine->gpu->deleteResource(ctx.handle_descrSet0);
	engine->gpu->deleteResource(ctx.handle_descrSet1);
    engine->gpu->deleteResource(ctx.handle_descrPool);
	
	engine->gpu->deleteResource (handle_descr_set_0);
	engine->gpu->deleteResource (handle_descr_set_1);

	ctx.allocator = NULL;
    ctx.engine = NULL;
}

//*******************************************
bool RenderPipe::priv_setup (gos::Allocator *allocatorIN, Engine *engineIN)
{
	engine = engineIN;
	gos::GPU *gpu = engineIN->gpu;
	ctx.allocator = allocatorIN;
	ctx.engine = engineIN;

	//descrivo i descriptor set 0 & 1 che sono comuni a tutte le PIPE
	gpu::Pipeline_def def;
	def
		.reset()
        .descriptorset_add(VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT)
            .add (0, eGPUDescriptrorType::SAMPLER, 3, eGPUDescriptrorUsage::vtx_shader | eGPUDescriptrorUsage::pxl_shader)
			.add (1, eGPUDescriptrorType::TEXTURE2D, 1024, eGPUDescriptrorUsage::vtx_shader | eGPUDescriptrorUsage::pxl_shader)
            .endDescriptorSet()
        .descriptorset_add()
            .add (0, eGPUDescriptrorType::UNIFORM_BUFFER, 1, eGPUDescriptrorUsage::vtx_shader | eGPUDescriptrorUsage::pxl_shader)
            .endDescriptorSet()
		;

	engine->gpu->descrSetLayout_create (def.descriptorSetList[0], &handle_descr_set_0);
	engine->gpu->descrSetLayout_create (def.descriptorSetList[1], &handle_descr_set_1);



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

        //sampler2d: bilinear filtering : REPEAT
        desc.reset();
		desc.addressModeU = eSamplerAddressMode::REPEAT;
		desc.addressModeV = eSamplerAddressMode::REPEAT;
        gpu->sampler_create (desc, &handle_samplers[2]);

    }

    //UBO "scene"
    gpu->uniformBuffer_create (sizeof(SceneData), eMemAccessMode::shared_cpuW_autoSync, &ctx.handle_ubo_scene);


	//altre risorse
	renderer_list.setup (ctx.allocator, 16);


	//alloco una istanza dei descriptor-set
	gos::gpu::DescrSetInstanceWriter dsw;

	//descriptor set 0
	if (!gpu->descrSetInstance_create (ctx.handle_descrPool, handle_descr_set_0, &ctx.handle_descrSet0))
	{
		gos::logger::err ("RenderPipe::setup() => can't create an instance of descriptorSet_0\n");
		return false;
	}
	else
	{
		dsw.begin (gpu, ctx.handle_descrSet0)
			.bindSamplerInArray  (0, handle_samplers[0], 0)             //bindo in samplerList[0] il sampler "bilinear"
			.bindSamplerInArray  (0, handle_samplers[1], 1)             //bindo in samplerList[1] il sampler "point"
			.bindSamplerInArray  (0, handle_samplers[2], 2)             //bindo in samplerList[2] il sampler "bilinear" con texture repeat
			.end();
	}

	//descriptor set 1
	if (!gpu->descrSetInstance_create (ctx.handle_descrPool, handle_descr_set_1, &ctx.handle_descrSet1))
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

	return true;
}

//*******************************************
bool RenderPipe::texture_add_reserved (GPUTextureHandle texHandle, u32 texture_index)
{
	const bool ret = texture_array.add_reserved (texHandle, texture_index);
	assert (ret);
	return ret;
}

//*******************************************
u32	RenderPipe::texture_add_if_dont_exists (GPUTextureHandle texHandle)
{ 
	assert (ctx.handle_descrSet0.isValid());

    bool bWasNew;
    const u32 texture_index = texture_array.add_if_dont_exists(texHandle, &bWasNew);
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
	//ad ogni renderer associo un UID progressivo che serve a loro per conoscere quale slot di
	//materialPBR->renderer_bindings[] utilizzare per storare informazioni personali
	assert (next_renderer_UID < gos::res::MaterialPBR::NUM_MAX_RENDERER);

	const u8 renderer_UID = next_renderer_UID++;
	renderer_list.append(r);
	r->on__attach(ctx, renderer_UID); 
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


	//rendering
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

