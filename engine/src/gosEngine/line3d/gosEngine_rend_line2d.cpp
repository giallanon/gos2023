#include "gosEngine_rend_line2d.h"
#include "../gosShape/gosShapeVtxArrayWriter.h"
#include "../gosEngine.h"

using namespace gos;
using namespace gos::engine;


//********************************************* 
Rend_line2d::Rend_line2d()
{
	engine = NULL;
    localAllocator = NULL;
}

//********************************************* 
Rend_line2d::~Rend_line2d()
{
	unsetup();
}

//********************************************* 
void Rend_line2d::unsetup()
{
    if (NULL == engine)
        return;

    gpu->buffer_unmap (sbo_segment.mapped);
    gpu->deleteResource (sbo_segment.gpu_handle);

    gpu->buffer_unmap (sbo_vtx.mapped);
    gpu->deleteResource (sbo_vtx.gpu_handle);

    engine->release(handle_shape_segmento);

    engine = NULL;
    localAllocator = NULL;
}

//********************************************* 
bool Rend_line2d::setup (gos::Allocator *allocatorIN, gos::Engine *engineIN)
{
    localAllocator = allocatorIN;
    engine = engineIN;
    gpu = engine->gpu;

    //load degli assets
    if (!engine->pipeline_createFromAsset ("gosengine_line2d", &handle_pipeline, engine::eLoadMode::asap))
        return false;    

    //creo gli oggetti che poi dovro' bindare ai descrittori
    {
        ///SBO con gli index ai vertici da usare per ogni singolo segmento di linea
        sbo_segment.size = NUM_MAX_SEGMENT_IN_BUFFER * sizeof(u32) * 2;
        gpu->storageBuffer_create (sbo_segment.size, eMemAccessMode::shared_cpuW_manualSync, &sbo_segment.gpu_handle);
        gpu->map (sbo_segment.gpu_handle, 0, u32MAX, &sbo_segment.mapped);

        //SBO con i vertici
        sbo_vtx.size = NUM_MAX_VTX_IN_BUFFER * sizeof(vec3f);
        gpu->storageBuffer_create (sbo_vtx.size, eMemAccessMode::shared_cpuW_manualSync, &sbo_vtx.gpu_handle);
        gpu->map (sbo_vtx.gpu_handle, 0, u32MAX, &sbo_vtx.mapped);
    }


    //creo un rect composto da 4 vtx e 6 index che serve come istanza per i segmenti di linea
    {
        const vec2f vtxList[4] = {
            vec2f(0, 0.5f),
            vec2f(1, 0.5f),
            vec2f(1, -0.5f),
            vec2f(0, -0.5f)
        };

        const u16 idxList[6] = {
            0,1,2,
            2,3,0
        };

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
    }



	return false;
}



//********************************************* 
void Rend_line2d::appendToCommandBuffer (Ctx *ctx, gos::gpu::pipe2::CmdBufferWriter2::BeginRend &rend)
{
    const engine::ResPipeline *res_pipeline;
    if (!engine->get (handle_pipeline, &res_pipeline))
        return;

    const engine::ResGPUShape *res_shape_segmento;
    if (!engine->get (handle_shape_segmento, &res_shape_segmento))
        return;

    //copio tutti i vtx is SBO
    const u32 num_vtx = ctx->vtxList.getNElem();
    if (0 == num_vtx)
        return;
    memcpy (sbo_vtx.mapped.host_pt,  ctx->vtxList._queryPointer(), sizeof(vec3f) * num_vtx);
    gpu->buffer_manualSync_cpuWrite (sbo_vtx.mapped, 0, sizeof(vec3f) * num_vtx);


    u32 *pt_segment_buffer = static_cast<u32*>(sbo_segment.mapped.host_pt);
    u32 iSeg = 0;

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

        case Ctx::eCMD::line_def:
            //inizio di una linea
            {
                u16 num_vtx_in_linea = ctx->program(i++);
                u16 vtx_index_1 = ctx->program(i++);

                num_vtx_in_linea--;
                while (num_vtx_in_linea--)
                {
                    const u16 vtx_index_2 = ctx->program(i++);

                    pt_segment_buffer[iSeg++] = vtx_index_1;
                    pt_segment_buffer[iSeg++] = vtx_index_2;
                    vtx_index_1 = vtx_index_2;
                }

                assert (iSeg <= NUM_MAX_SEGMENT_IN_BUFFER*2);
            }
            break;
        }
    }

    //aggiorno SBO dei segmenti
    gpu->buffer_manualSync_cpuWrite (sbo_segment.mapped, 0, sizeof(u32) * iSeg);

    rend
		.bindPipeline (res_pipeline->data.pipeHandle)
		//.bindDescriptorSet (handle_descrSet0, 0)
		//.bindDescriptorSet (handle_descrSet1, 1)
		//.bindDescriptorSet (handle_descrSet2, 2);
        .drawIndexed (res_shape_segmento->numIndices, iSeg/2, res_shape_segmento->indexStart, res_shape_segmento->vtxStart, 0);


}




