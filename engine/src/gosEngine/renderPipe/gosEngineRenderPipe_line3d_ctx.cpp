#include "gosEngineRenderPipe_line3d.h"
#include "../gosEngine.h"

using namespace gos;
using namespace gos::engine;


//**************************************************
Renderer_line3d::Ctx::Ctx()
{
	line_started_at = u32MAX;
}

//**************************************************
void Renderer_line3d::Ctx::setup (gos::Allocator *allocator, u16 estimated_num_vtx)
{
	vtxList.setup (allocator, estimated_num_vtx);
	program.setup (allocator, 1024);
}

//**************************************************
void Renderer_line3d::Ctx::unsetup ()
{
	vtxList.unsetup();
	program.unsetup();
}


//********************************************* 
Renderer_line3d::Ctx& Renderer_line3d::Ctx::clear()
{
	vtxList.reset();
	program.reset();
	line_started_at = u32MAX;
	enable_depth_test(false);
	enable_depth_write(false);
	set_line_width(3);
	return *this;
}

//********************************************* 
u16 Renderer_line3d::Ctx::vtx_add (const vec3f &p)
{
	const u32 ret = vtxList.getNElem();
	vtxList[ret] = p;
	return (u16)ret;
}


//********************************************* 
Renderer_line3d::Ctx& Renderer_line3d::Ctx::set_color_ARGB (u32 argb)
{
	program.append ((u16)eCMD::set_color_ARGB);
	program.append ((u16)  ((argb & 0xFFFF0000) >> 16) );
	program.append ( (argb & 0x0000FFFF) );
	return *this;
}

//********************************************* 
Renderer_line3d::Ctx& Renderer_line3d::Ctx::enable_depth_test(bool b)
{
	if (b)
		program.append ((u16)eCMD::enable_depth_test);
	else
		program.append ((u16)eCMD::disable_depth_test);

	return *this;
}

//********************************************* 
Renderer_line3d::Ctx& Renderer_line3d::Ctx::set_line_width (u16 w)
{
	program.append ((u16)eCMD::set_line_width);
	program.append (w);

	return *this;
}

//********************************************* 
Renderer_line3d::Ctx& Renderer_line3d::Ctx::enable_depth_write(bool b)
{
	if (b)
		program.append ((u16)eCMD::enable_depth_write);
	else
		program.append ((u16)eCMD::disable_depth_write);

	return *this;
}


//********************************************* 
Renderer_line3d::Ctx& Renderer_line3d::Ctx::line_begin()
{
	assert (u32MAX == line_started_at);
	line_started_at = program.getNElem();
	program[line_started_at] = (u16)eCMD::line_def;
	program[line_started_at+1] = 0;		//num vtx in questa linea: lo fillo in line_end()

	return *this;
}

//********************************************* 
Renderer_line3d::Ctx& Renderer_line3d::Ctx::line_add_vtx (u16 vtx_index)
{
	assert (u32MAX != line_started_at);
	program.append (vtx_index);
	return *this;
}

//********************************************* 
Renderer_line3d::Ctx& Renderer_line3d::Ctx::line_end()
{
	assert (u32MAX != line_started_at);
	const u16 num_vtx = (u16) (program.getNElem() - line_started_at - 2);
	program[line_started_at+1] = num_vtx;
	line_started_at = u32MAX;
	return *this;
}

/********************************************* 
 * dati <num_vtx> in <vtxList>, li adda tutti e poi disegna una linea chiusa tra tutti questi vtx
 */
Renderer_line3d::Ctx& Renderer_line3d::Ctx::closed_line (const FastArray<vec3f> &vtxList, u32 num_vtx)
{
	u16 first_vtx_index = vtx_add (vtxList(0));
	for (u32 i=1; i<num_vtx; i++)
		vtx_add (vtxList(i));

	line_begin();
	for (u32 i=0; i<num_vtx; i++)
		line_add_vtx (first_vtx_index++);
	line_add_vtx (first_vtx_index - num_vtx);
	line_end();

	return *this;
}

//********************************************* 
Renderer_line3d::Ctx& Renderer_line3d::Ctx::point_set_radius (u16 radius)
{
	program.append ((u16)eCMD::set_point_radius);
	program.append (radius);
	return *this;
}

//********************************************* 
Renderer_line3d::Ctx& Renderer_line3d::Ctx::point (u16 vtx_index)
{
	program.append ((u16)eCMD::point_def);
	program.append (vtx_index);
	return *this;
}

//********************************************* 
Renderer_line3d::Ctx& Renderer_line3d::Ctx::aabb3 (const vec3f &vmin, const vec3f &vmax, u16 line_width)
{
	if (0 != line_width)
		set_line_width (line_width);

	const u16 idx[8] = {
		vtx_add(vmin.x, vmin.y, vmin.z),
		vtx_add(vmax.x, vmin.y, vmin.z),
		vtx_add(vmax.x, vmax.y, vmin.z),
		vtx_add(vmin.x, vmax.y, vmin.z),

		vtx_add(vmin.x, vmin.y, vmax.z),
		vtx_add(vmax.x, vmin.y, vmax.z),
		vtx_add(vmax.x, vmax.y, vmax.z),
		vtx_add(vmin.x, vmax.y, vmax.z)
	};

	line_begin();
		line_add_vtx(idx[0]);
		line_add_vtx(idx[1]);
		line_add_vtx(idx[2]);
		line_add_vtx(idx[3]);
		line_add_vtx(idx[0]);
	line_end();

	line_begin();
		line_add_vtx(idx[4]);
		line_add_vtx(idx[5]);
		line_add_vtx(idx[6]);
		line_add_vtx(idx[7]);
		line_add_vtx(idx[4]);
	line_end();

	line (idx[0], idx[4]);
	line (idx[1], idx[5]);
	line (idx[2], idx[6]);
	line (idx[3], idx[7]);
	

	return *this;
}