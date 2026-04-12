#include "gosEngine_rend_line3d.h"
#include "../gosEngine.h"

using namespace gos;
using namespace gos::engine;


//**************************************************
Rend_line3d::Ctx::Ctx()
{
	line_started_at = u32MAX;
}

//**************************************************
void Rend_line3d::Ctx::setup (gos::Allocator *allocator, u16 estimated_num_vtx)
{
	vtxList.setup (allocator, estimated_num_vtx);
	program.setup (allocator, 1024);
}

//**************************************************
void Rend_line3d::Ctx::unsetup ()
{
	vtxList.unsetup();
	program.unsetup();
}


//********************************************* 
void Rend_line3d::Ctx::clear()
{
	vtxList.reset();
	program.reset();
	line_started_at = u32MAX;
	enable_depth_test(false);
	enable_depth_write(false);
	set_line_width(3);
}

//********************************************* 
u16 Rend_line3d::Ctx::vtx_add (const vec3f &p)
{
	const u32 ret = vtxList.getNElem();
	vtxList[ret] = p;
	return (u16)ret;
}


//********************************************* 
void Rend_line3d::Ctx::set_color_ARGB (u32 argb)
{
	program.append ((u16)eCMD::set_color_ARGB);
	program.append ((u16)  ((argb & 0xFFFF0000) >> 16) );
	program.append ( (argb & 0x0000FFFF) );
}

//********************************************* 
void Rend_line3d::Ctx::enable_depth_test(bool b)
{
	if (b)
		program.append ((u16)eCMD::enable_depth_test);
	else
		program.append ((u16)eCMD::disable_depth_test);
}

//********************************************* 
void Rend_line3d::Ctx::set_line_width (u16 w)
{
	program.append ((u16)eCMD::set_line_width);
	program.append (w);
}

//********************************************* 
void Rend_line3d::Ctx::enable_depth_write(bool b)
{
	if (b)
		program.append ((u16)eCMD::enable_depth_write);
	else
		program.append ((u16)eCMD::disable_depth_write);
}


//********************************************* 
void Rend_line3d::Ctx::line_begin()
{
	assert (u32MAX == line_started_at);
	line_started_at = program.getNElem();
	program[line_started_at] = (u16)eCMD::line_def;
	program[line_started_at+1] = 0;		//num vtx in questa linea: lo fillo in line_end()
}

//********************************************* 
void Rend_line3d::Ctx::line_add_vtx (u16 vtx_index)
{
	assert (u32MAX != line_started_at);
	program.append (vtx_index);
}

//********************************************* 
void Rend_line3d::Ctx::line_end()
{
	assert (u32MAX != line_started_at);
	const u16 num_vtx = (u16) (program.getNElem() - line_started_at - 2);
	program[line_started_at+1] = num_vtx;
	line_started_at = u32MAX;
}

/********************************************* 
 * dati <num_vtx> in <vtxList>, li adda tutti e poi disegna una linea chiusa tra tutti questi vtx
 */
void Rend_line3d::Ctx::closed_line (const FastArray<vec3f> &vtxList, u32 num_vtx)
{
	u16 first_vtx_index = vtx_add (vtxList(0));
	for (u32 i=1; i<num_vtx; i++)
		vtx_add (vtxList(i));

	line_begin();
	for (u32 i=0; i<num_vtx; i++)
		line_add_vtx (first_vtx_index++);
	line_add_vtx (first_vtx_index - num_vtx);
	line_end();
}

//********************************************* 
void Rend_line3d::Ctx::point_set_radius (u16 radius)
{
	program.append ((u16)eCMD::set_point_radius);
	program.append (radius);
}

//********************************************* 
void Rend_line3d::Ctx::point (u16 vtx_index)
{
	program.append ((u16)eCMD::point_def);
	program.append (vtx_index);
}