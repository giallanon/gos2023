#include "gosEngine_rend_line3d.h"
#include "../gosShape/gosShapeVtxArrayWriter.h"
#include "../gosEngine.h"

using namespace gos;
using namespace gos::engine;


//********************************************* 
Rend_line3d::Rend_line3d()
{
	engine = NULL;
    localAllocator = NULL;
}

//********************************************* 
Rend_line3d::~Rend_line3d()
{
	unsetup();
}

//********************************************* 
void Rend_line3d::unsetup()
{
    if (NULL == engine)
        return;

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
bool Rend_line3d::setup (gos::Allocator *allocatorIN, gos::Engine *engineIN)
{
    localAllocator = allocatorIN;
    engine = engineIN;
    gpu = engine->gpu;

    //load degli assets
    if (!engine->pipeline_createFromAsset ("gosengine_line3d", &handle_pipeline, engine::eLoadMode::asap))
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

        gos::shape::VtxArrayWriter::Elem<vec2f> vtx;
        writer.getPos2 (&vtx);
        vtx().set(0, 0.5f);	    vtx.next();
        vtx().set(1, 0.5f);	    vtx.next();
        vtx().set(1, -0.5f);	vtx.next();
        vtx().set(0, -0.5f);	vtx.next();

        writer.addTris (0, 1, 2);
        writer.addTris (2, 3, 0);
    }

    engine->utils__quick_and_dirty__create_GPUSHape_and_stageIt_to_VB_IB (&shape, &handle_shape_segmento);
    shape::shapeFree (gos::getScrapAllocator(), &shape);



    //creo un descriptor pool
    gpu->descrPool_createNew (&handle_descrPool)
        .setMaxNumDescriptorSet(2)
        .addPool_uniformBuffer(1)
        .addPool_storageBuffer(2)
        .end();
    if (handle_descrPool.isInvalid())
    {
        gos::logger::err ("Rend_line3d::setup() => can't create descriptor pool\n");
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
    const engine::ResPipeline *res_pipeline;
    if (engine->get (handle_pipeline, &res_pipeline, 5000))
    {
        //alloco una istanza dei descriptor-set
        gos::gpu::DescrSetInstanceWriter dsw;

        //descriptor set 0
        if (!gpu->descrSetInstance_create (handle_descrPool, res_pipeline->data.pipeHandle, 0, &handle_descrSet0))
        {
            gos::logger::err ("Rend_line3d::setup() => can't create an instance of descriptorSet_0\n");
            return false;
        }
        else
        {
            dsw.begin (gpu, handle_descrSet0)
                .bindUniformBuffer (0, handle_ubo_scene, 0)
                .end();
        }
        

        //descriptor set 1
        if (!gpu->descrSetInstance_create (handle_descrPool, res_pipeline->data.pipeHandle, 1, &handle_descrSet1))
        {
            gos::logger::err ("Rend_line3d::setup() => can't create an instance of descriptorSet_0\n");
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

	return true;
}


//********************************************* 
void Rend_line3d::priv_flushProgram (sState &state, const engine::ResGPUShape *res_shape_segmento, gos::gpu::CmdBufferWriter2::BeginRend &cwr)
{
    if (0 == state.num_seg_to_draw)
        return;

    gos::ColorHDR col_RGBA;
    col_RGBA.setU32_argb (state.cur_color_ARGB);

    cwr
        .setDepthTestEnable(state.bDepthTestEnabled)
        .setDepthWriteEnable(state.bDepthWriteEnabled)
        .pushConstant (0, &col_RGBA.col.rgba, sizeof(col_RGBA.col.rgba))
        .drawIndexed (res_shape_segmento->numIndices, state.num_seg_to_draw, res_shape_segmento->indexStart, res_shape_segmento->vtxStart, state.first_instance_index);

    state.first_instance_index += state.num_seg_to_draw;
    state.num_seg_to_draw = 0;
}

//********************************************* 
void Rend_line3d::appendToCommandBuffer (Ctx *ctx, gos::gpu::CmdBufferWriter2::BeginRend &cwr, gos::geom::Camera3 *cam)
{
    const engine::ResPipeline *res_pipeline;
    if (!engine->get (handle_pipeline, &res_pipeline))
        return;

    const engine::ResGPUShape *res_shape_segmento;
    if (!engine->get (handle_shape_segmento, &res_shape_segmento))
        return;

    //default rendering params
    sState state;
    state.num_seg_to_draw = 0;
    state.first_instance_index = 0;
    state.cur_color_ARGB = 0xFFFF00FF;
    state.bDepthTestEnabled = false;
    state.bDepthWriteEnabled = false;


    cwr
		.bindPipeline (res_pipeline->data.pipeHandle)
		.bindDescriptorSet (handle_descrSet0, 0)
		.bindDescriptorSet (handle_descrSet1, 1)
        .bindVtxIdxBuffer (res_shape_segmento->vbHandle, 0, res_shape_segmento->ibHandle, 0);



    SceneData scene;
	scene.matVP = cam->getMatVP();
	scene.screen_wh.set (gpu->swapChain_getWidth(), gpu->swapChain_getHeight());
	gpu->writeAndSync (handle_ubo_scene, 0, &scene, sizeof(scene));

    //copio tutti i vtx is SBO
    const u32 num_vtx = ctx->vtxList.getNElem();
    if (0 == num_vtx)
        return;
    memcpy (sbo_vtx.mapped.host_pt,  ctx->vtxList._queryPointer(), sizeof(vec3f) * num_vtx);
    gpu->buffer_manualSync_cpuWrite (sbo_vtx.mapped, 0, u32MAX);
    
    

    //parse del program
    u32 *pt_segment_buffer = static_cast<u32*>(sbo_segment.mapped.host_pt);
    u32 segment_buffer_ct = 0;
    u32 n = ctx->program.getNElem();
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
                    priv_flushProgram (state, res_shape_segmento, cwr);
                    state.cur_color_ARGB = argb;
                }
            }
            break;

        case Ctx::eCMD::enable_depth_test:
            if (state.bDepthTestEnabled != true)
            {
                priv_flushProgram (state, res_shape_segmento, cwr);
                state.bDepthTestEnabled = true;
            }
            break;

        case Ctx::eCMD::disable_depth_test:
            if (state.bDepthTestEnabled != false)
            {
                priv_flushProgram (state, res_shape_segmento, cwr);
                state.bDepthTestEnabled = false;
            }
            break;        

        case Ctx::eCMD::enable_depth_write:
            if (state.bDepthWriteEnabled != true)
            {
                priv_flushProgram (state, res_shape_segmento, cwr);
                state.bDepthWriteEnabled = true;
            }
            break;         
            
        case Ctx::eCMD::disable_depth_write:
            if (state.bDepthWriteEnabled != false)
            {
                priv_flushProgram (state, res_shape_segmento, cwr);
                state.bDepthWriteEnabled = false;
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

                    const u32 packed_idx_1_2 = (u32)vtx_index_1 << 8 | vtx_index_2;
                    pt_segment_buffer[segment_buffer_ct++] = packed_idx_1_2;
                    state.num_seg_to_draw++;
                    vtx_index_1 = vtx_index_2;
                }

                assert (state.num_seg_to_draw <= NUM_MAX_SEGMENT_IN_BUFFER);
            }
            break;
        }
    }

    priv_flushProgram (state, res_shape_segmento, cwr);


    //aggiornamento dell SSBO contenente i segmenti
    gpu->buffer_manualSync_cpuWrite (sbo_segment.mapped, 0, u32MAX);
    cwr.endRender();
}




