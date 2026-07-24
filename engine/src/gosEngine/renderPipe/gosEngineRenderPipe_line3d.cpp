#include "gosEngineRenderPipe_line3d.h"
#include "../gosShape/gosShapeVtxArrayWriter.h"
#include "../gosEngine.h"

using namespace gos;
using namespace gos::engine;


//********************************************* 
Renderer_line3d::Renderer_line3d()
{
	engine = NULL;
    localAllocator = NULL;
    flag.zero();
}

//********************************************* 
Renderer_line3d::~Renderer_line3d()
{
	priv_unsetup();
}

//********************************************* 
void Renderer_line3d::priv_unsetup()
{
    if (NULL == engine)
	{
        return;
	}

	u32 n = ctx_list.getNElem();
	for (u32 i=0; i<n; i++)
	{
		ctx_list[i].ctx->unsetup();
		GOSDELETE(localAllocator, ctx_list[i].ctx);
	}
	ctx_list.unsetup();


    gpu->buffer_unmap (sbo_segment.mapped);
    gpu->buffer_unmap (sbo_vtx.mapped);

    engine->release(handle_shape_segmento);

    
    engine->release(handle_pipeline);
        gpu->deleteResource(handle_ubo_scene);
        gpu->deleteResource(sbo_segment.gpu_handle);
        gpu->deleteResource(sbo_vtx.gpu_handle);
        gpu->deleteResource(handle_descrSet0);
        gpu->deleteResource(handle_descrSet1);
        gpu->deleteResource(handle_descrPool);


    engine = NULL;
    localAllocator = NULL;
}

//********************************************* 
bool Renderer_line3d::on__attach (const RPIPE::Context &ctx, u8 renderer_UID)
{
    localAllocator = ctx.allocator;
    engine = ctx.engine;
    gpu = engine->gpu;

    //load degli assets
    if (!engine->pipeline_createFromAsset ("gosengine_line3d", &handle_pipeline, res::eLoadMode::asap))
        return false;    

    //creo la shape del singolo segmento
    gos::Shape shape;
    {
        gos::VtxLayout vtxLayout;
        {
            shape::VtxLayoutWriter vtxLayoutW;

            vtxLayoutW.setup (&vtxLayout);
            vtxLayoutW.begin()
                .addPos2 (0)
                .end();
        }

        shape.reset();
        if (!shape::shapeAlloc (gos::getScrapAllocator(), vtxLayout, 4, 6, &shape))
            return false;

        gos::shape::VtxArrayWriter writer;
        writer.setup (&shape);


		/*
			(0.0, 0.5)               (1.0, 0.5)


			(0.0, -0.5)              (1.0, -0.5)
		*/
        gos::shape::VtxArrayWriter::Elem<vec2f> vtx;
        writer.getPos2 (&vtx);
        vtx().set(0, 0.5f);	    vtx.next();
        vtx().set(1, 0.5f);	    vtx.next();
        vtx().set(1, -0.5f);	vtx.next();
        vtx().set(0, -0.5f);	vtx.next();

        writer.addTris (0, 1, 2);
        writer.addTris (2, 3, 0);
    }


	gpu::StageHelper stageHelper;
	stageHelper.setup (gpu, 1024);
	engine->GPUShape_create (&shape, stageHelper, &handle_shape_segmento);
    shape::shapeFree (gos::getScrapAllocator(), &shape);



    //creo un descriptor pool
    gpu->descrPool_createNew (&handle_descrPool)
        .setMaxNumDescriptorSet(2)
        .addPool_uniformBuffer(1)
        .addPool_storageBuffer(2)
        .end();
    if (handle_descrPool.isInvalid())
    {
        gos::logger::err ("Renderer_line3d::setup() => can't create descriptor pool\n");
        return false;
    }    

    //creo gli oggetti che poi dovro' bindare ai descrittori
    {
        //UBO "scene"
        gpu->uniformBuffer_create (sizeof(SceneData), eMemAccessMode::shared_cpuW_autoSync, &handle_ubo_scene);

        
        ///SBO con gli index ai vertici da usare per ogni singolo segmento di linea
        sbo_segment.size = NUM_MAX_SEGMENT_IN_BUFFER * sizeof(u32);
        gpu->storageBuffer_create (sbo_segment.size, eMemAccessMode::shared_cpuW_manualSync, &sbo_segment.gpu_handle);
        gpu->map (sbo_segment.gpu_handle, 0, sbo_segment.size, &sbo_segment.mapped);

        //SBO con i vertici
        sbo_vtx.size = NUM_MAX_VTX_IN_BUFFER * sizeof(vec3f);
        gpu->storageBuffer_create (sbo_vtx.size, eMemAccessMode::shared_cpuW_manualSync, &sbo_vtx.gpu_handle);
        gpu->map (sbo_vtx.gpu_handle, 0, sbo_vtx.size, &sbo_vtx.mapped);
    }


    //attendo che la pipe sia stata caricata perche' mi servono le definizioni dei descrittori
    const res::Pipeline *res_pipeline;
    if (engine->get (handle_pipeline, &res_pipeline, 5000))
    {
        //alloco una istanza dei descriptor-set
        gos::gpu::DescrSetInstanceWriter dsw;

        //descriptor set 0
        if (!gpu->descrSetInstance_create (handle_descrPool, res_pipeline->pipeHandle, 0, &handle_descrSet0))
        {
            gos::logger::err ("Renderer_line3d::setup() => can't create an instance of descriptorSet_0\n");
            return false;
        }
        else
        {
            dsw.begin (gpu, handle_descrSet0)
                .bindUniformBuffer (0, handle_ubo_scene, 0)
                .end();
        }
        

        //descriptor set 1
        if (!gpu->descrSetInstance_create (handle_descrPool, res_pipeline->pipeHandle, 1, &handle_descrSet1))
        {
            gos::logger::err ("Renderer_line3d::setup() => can't create an instance of descriptorSet_0\n");
            return false;
        }
        else
        {
            dsw.begin (gpu, handle_descrSet1)
                .bindStorageBuffer (0, sbo_vtx.gpu_handle, 0)
                .bindStorageBuffer (1, sbo_segment.gpu_handle, 0)
                .end();
        }    
    }


	ctx_list.setup (localAllocator, 16);
	return true;
}

//********************************************* 
Renderer_line3d::Ctx* Renderer_line3d::ctx__create_new (const char *name, u16 estimated_num_vtx)
{
	if (u32MAX != priv_ctx__get(name))
	{
		logger::err ("Renderer_line3d::ctx__create_new() => %s already exists\n");
		return NULL;
	}

	Renderer_line3d::Ctx *ctx = GOSNEW(localAllocator, Renderer_line3d::Ctx)();
	ctx->setup (localAllocator, estimated_num_vtx);

	const u32 n = ctx_list.getNElem();
	ctx_list[n].ctx = ctx;
	sprintf_s (ctx_list[n].name, sizeof(ctx_list[n].name), "%s", name);
	return ctx;
}

//********************************************* 
void Renderer_line3d::ctx__delete (const char *name)
{
	const u32 n = priv_ctx__get(name);
	if (u32MAX == n)
		return;

	ctx_list[n].ctx->unsetup();
	GOSDELETE(localAllocator, ctx_list[n].ctx);
	ctx_list.removeAndSwapWithLast(n);
}

//********************************************* 
u32 Renderer_line3d::priv_ctx__get (const char *name) const
{
	const u32 n = ctx_list.getNElem();
	for (u32 i=0; i<n; i++)
	{
		if (string::utf8::areEqual (name, ctx_list(i).name, true))
			return i;
	}
	return u32MAX;
}

//********************************************* 
Renderer_line3d::Ctx* Renderer_line3d::ctx__get (const char *name)
{
	const u32 i = priv_ctx__get(name);
	if (u32MAX == i)
	{
		DBGBREAK;
		return NULL;
	}
	return ctx_list(i).ctx;
}

//********************************************* 
void Renderer_line3d::on__render (const RPIPE::Context &ctx, gos::gpu::RenderCtx &rctx)
{
	const u32 n = ctx_list.getNElem();
	if (0 == n)
	{
		return;
	}

	priv_begin (ctx.cam, &rctx);
	for (u32 i=0; i<n; i++)
	{
		priv_appendToCommandBuffer (ctx_list(i).ctx);
	}
	priv_end();
}

//********************************************* 
void Renderer_line3d::priv_begin (gos::geom::Camera3 *cam, gpu::RenderCtx *rctxIN)
{
    if (flag.isBitSet(FLAG__BEGIN_INVOKED))
    {
        DBGBREAK;
        return;
    }

    const res::Pipeline *res_pipeline;
    if (!engine->get (handle_pipeline, &res_pipeline))
        return;

    if (!engine->get (handle_shape_segmento, &res_shape_segmento))
        return;   
        
        
    flag.set(FLAG__BEGIN_INVOKED);
    rctx = rctxIN;
    num_vtx_in_buffer = 0;
    num_seg_in_buffer = 0;

    //default rendering params
    state.num_seg_to_draw = 0;
    state.first_instance_index = 0;
    state.cur_line_width = 3;
	state.cur_point_radius = 2;
    state.cur_color_ARGB = 0xFFFF00FF;
    state.bDepthTestEnabled = false;
    state.bDepthWriteEnabled = false;


    (*rctx)
		.bindPipeline (res_pipeline->pipeHandle)
		.bindDescriptorSet (handle_descrSet0, 0)
		.bindDescriptorSet (handle_descrSet1, 1)
        .bindVtxIdxBuffer (res_shape_segmento->vbHandle, 0, res_shape_segmento->ibHandle, 0);
 

    SceneData scene;
	scene.matVP = cam->getMatVP();
	scene.screen_wh.set ((f32)gpu->swapChain_getWidth(), (f32)gpu->swapChain_getHeight());
	gpu->writeAndSync (handle_ubo_scene, 0, &scene, sizeof(scene));

}

//********************************************* 
void Renderer_line3d::priv_end()
{
    if (!flag.isBitSet(FLAG__BEGIN_INVOKED))
	{
    	return;
	}

    flag.clear(FLAG__BEGIN_INVOKED);
    gpu->buffer_manualSync_cpuWrite (sbo_vtx.mapped, 0, u32MAX);
    gpu->buffer_manualSync_cpuWrite (sbo_segment.mapped, 0, u32MAX);
	//gpu->waitIdle();
    rctx = NULL;
    res_shape_segmento = NULL;
}

//********************************************* 
void Renderer_line3d::priv_flushProgram (sState &state)
{
    if (0 == state.num_seg_to_draw)
        return;

    gos::ColorHDR col_RGBA;
    col_RGBA.setU32_argb (state.cur_color_ARGB);

    (*rctx)
        .setDepthTestEnable(state.bDepthTestEnabled)
        .setDepthWriteEnable(state.bDepthWriteEnabled)
        .pushConstant (0, &col_RGBA.col.rgba, sizeof(col_RGBA.col.rgba))
        .pushConstant (1, &state.cur_line_width, sizeof(state.cur_line_width))
		.pushConstant (2, &state.cur_point_radius, sizeof(state.cur_point_radius))
        .drawIndexed (res_shape_segmento->numIndices, state.num_seg_to_draw, res_shape_segmento->indexStart, res_shape_segmento->vtxStart, state.first_instance_index);

    state.first_instance_index += state.num_seg_to_draw;
    state.num_seg_to_draw = 0;
}

//********************************************* 
void Renderer_line3d::priv_appendToCommandBuffer (const Ctx *ctx)
{
    if (!flag.isBitSet(FLAG__BEGIN_INVOKED))
    {
        DBGBREAK;
        return;
    }

    //default rendering params
    state.num_seg_to_draw = 0;
    state.first_instance_index = num_seg_in_buffer;

    //copio tutti i vtx di questo ctx in SBO
    const u32 first_vtx_index = num_vtx_in_buffer;
    const u32 num_vtx = ctx->vtxList.getNElem();
    if (0 == num_vtx)
        return;
    else
    {
        const u32 offset = num_vtx_in_buffer*sizeof(vec3f);
        const u32 size =  sizeof(vec3f) * num_vtx;

        u8 *pt = static_cast<u8*>(sbo_vtx.mapped.host_pt);
        memcpy (&pt[offset], ctx->vtxList._queryPointer(), size);
        //gpu->buffer_manualSync_cpuWrite (sbo_vtx.mapped, offset, size);

        num_vtx_in_buffer += num_vtx;
    }
    
    

    //parse del program di ctx
    u32 *pt_segment_buffer = static_cast<u32*>(sbo_segment.mapped.host_pt);
    
    const u32 n = ctx->program.getNElem();
    u32 i = 0;
    while (i < n)
    {
        const Ctx::eCMD cmd = (Ctx::eCMD)ctx->program(i++);
        switch (cmd)
        {
        default:
            DBGBREAK;
            return;

        case Ctx::eCMD::set_color_ARGB:
            {
                u32 argb = (u32) (ctx->program(i++) << 16);
                argb |= (u32) (ctx->program(i++));;

                if (argb != state.cur_color_ARGB)
                {
                    priv_flushProgram (state);
                    state.cur_color_ARGB = argb;
                }
            }
            break;

        case Ctx::eCMD::enable_depth_test:
            if (state.bDepthTestEnabled != true)
            {
                priv_flushProgram (state);
                state.bDepthTestEnabled = true;
            }
            break;

        case Ctx::eCMD::disable_depth_test:
            if (state.bDepthTestEnabled != false)
            {
                priv_flushProgram (state);
                state.bDepthTestEnabled = false;
            }
            break;        

        case Ctx::eCMD::enable_depth_write:
            if (state.bDepthWriteEnabled != true)
            {
                priv_flushProgram (state);
                state.bDepthWriteEnabled = true;
            }
            break;         
            
        case Ctx::eCMD::disable_depth_write:
            if (state.bDepthWriteEnabled != false)
            {
                priv_flushProgram (state);
                state.bDepthWriteEnabled = false;
            }
            break;             
        
        case Ctx::eCMD::set_line_width:
            {
                const u16 w = ctx->program(i++);
                if (w != state.cur_line_width)
                {
                    priv_flushProgram (state);
                    state.cur_line_width = w;
                }
            }
            break;

        case Ctx::eCMD::line_def:
            //inizio di una linea
            {
                u16 num_vtx_in_linea = ctx->program(i++);
                u16 vtx_index_1 = ctx->program(i++);

                num_vtx_in_linea--;
                while (num_vtx_in_linea--)
                {
                    const u16 vtx_index_2 = ctx->program(i++);

                    const u32 packed_idx_1_2 = (u32)(first_vtx_index + vtx_index_1) << 16 | (first_vtx_index + vtx_index_2);
                    pt_segment_buffer[num_seg_in_buffer++] = packed_idx_1_2;

                    state.num_seg_to_draw++;

                    vtx_index_1 = vtx_index_2;
                }

                assert (num_seg_in_buffer <= NUM_MAX_SEGMENT_IN_BUFFER);
            }
            break;

		case Ctx::eCMD::set_point_radius:
            {
                const u16 w = ctx->program(i++);
                if (w != state.cur_point_radius)
                {
                    priv_flushProgram (state);
                    state.cur_point_radius = w;
                }
            }
            break;		

		case Ctx::eCMD::point_def:
            {
                const u16 vtx_index_2 = ctx->program(i++);

				const u32 packed_idx_1_2 = 0xFFFF0000 | (first_vtx_index + vtx_index_2);
				pt_segment_buffer[num_seg_in_buffer++] = packed_idx_1_2;

				state.num_seg_to_draw++;
				assert (num_seg_in_buffer <= NUM_MAX_SEGMENT_IN_BUFFER);
            }
            break;
        }
    }

    priv_flushProgram (state);


    //aggiornamento dell SSBO contenente i segmenti
    //gpu->buffer_manualSync_cpuWrite (sbo_segment.mapped, 0, u32MAX);
}




