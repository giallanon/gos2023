#include "gosEngine_line3d.h"
#include "../gosEngine.h"

using namespace gos;


//********************************************* 
Line3DCtx::Line3DCtx()
{
	allocator = NULL;
}

//********************************************* 
void Line3DCtx::unsetup ()
{
	if (NULL == allocator)
		return;

	pointList.unsetup ();
	program.unsetup();
	allocator = NULL;
}

//********************************************* 
void Line3DCtx::setup (gos::Allocator *allocatorIN, u16 estimated_num_point)
{
	assert (NULL == allocator);

	pointList.setup (allocator, estimated_num_point);
	program.setup (allocator, 1024);
}

//********************************************* 
void Line3DCtx::reset()
{
	pointList.reset();
	program.reset();
	line_started_at = u32MAX;
}

//********************************************* 
u16 Line3DCtx::point_add (const vec3f &p)
{
	const u32 n = pointList.getNElem();
	pointList[n] = p;
	return (u16)n;
}

//********************************************* 
void Line3DCtx::line_begin()
{
	assert (u32MAX == line_started_at);
	line_started_at = program.getNElem();
	
	program.append ((u16)eCMD::line_def); //segnalo che da qui in poi c'e' la definizione di una linea
	program.append (0); //composta da quanti vertici?  (lo fillo in line_end())

}

//********************************************* 
void Line3DCtx::line_add_point (u16 point_index)
{
	assert (u32MAX != line_started_at);
	program.append (point_index);
}

//********************************************* 
void Line3DCtx::line_end()
{
	assert (u32MAX != line_started_at);
	
	//conto il numero di vertici inseriti e lo inserisco in program subito dopo eCMD::line_def
	const u32 num_vtx = program.getNElem() - line_started_at - 2;
	program[line_started_at+1] = (u16)num_vtx;
	line_started_at = u32MAX;
}

//********************************************* 
void Line3DCtx::fillCommandBuffer (gos::gpu::pipe2::CmdBufferWriter2 &cw)
{
	//auto &renderer = cw.beginRender();
 //   renderer.withRenderArea (handle_rt0)
 //           .withRT (handle_rt0, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0.0f, 0.1f))
 //           .withZB (handle_zbuffer, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care)
	//		.bindPipeline (res_pipeline->data.pipeHandle)
	//		.bindDescriptorSet (handle_descrSet0, 0)
	//		.bindDescriptorSet (handle_descrSet1, 1)
	//		.bindDescriptorSet (handle_descrSet2, 2);
}