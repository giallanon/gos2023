#include "gosEngine_rend_line2d.h"
#include "../gosEngine.h"

using namespace gos;
using namespace gos::engine;


//**************************************************
Rend_line2d::Ctx::Ctx()
{
	line_started_at = u32MAX;
}

//**************************************************
void Rend_line2d::Ctx::setup (gos::Allocator *allocator, u16 estimated_num_vtx)
{
	vtxList.setup (allocator, estimated_num_vtx);
	program.setup (allocator, 1024);
}

//**************************************************
void Rend_line2d::Ctx::unsetup ()
{
	vtxList.unsetup();
	program.unsetup();
}


//********************************************* 
void Rend_line2d::Ctx::clear()
{
	vtxList.reset();
	program.reset();
	line_started_at = u32MAX;
}

//********************************************* 
u16 Rend_line2d::Ctx::vtx_add (const vec3f &p)
{
	const u32 ret = vtxList.getNElem();
	vtxList[ret] = p;
	return (u16)ret;
}

//********************************************* 
void Rend_line2d::Ctx::line_begin()
{
	assert (u32MAX == line_started_at);
	line_started_at = program.getNElem();
	program[line_started_at] = (u16)eCMD::line_def;
	program[line_started_at+1] = 0;		//num vtx in questa linea: lo fillo in line_end()
}

//********************************************* 
void Rend_line2d::Ctx::line_add_vtx (u16 vtx_index)
{
	assert (u32MAX != line_started_at);
	program.append (vtx_index);
}

//********************************************* 
void Rend_line2d::Ctx::line_end()
{
	assert (u32MAX != line_started_at);
	const u16 num_vtx = (u16) (program.getNElem() - line_started_at - 2);
	program[line_started_at+1] = num_vtx;
}