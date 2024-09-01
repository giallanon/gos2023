#include "gosShapePrefabs.h"
#include "gosShape.h"


using namespace gos;
using namespace gos::shape;


//*************************************************************
//bool shape::buildCube24 (const vec3f &center, const vec3f &size, VtxArrayWriter *writer, Info *out_info)
bool shape::buildCube24 (const vec3f &center, const vec3f &size, const VtxLayout &vtxLayout, gos::Allocator *shapeAllocator, Shape *out_shape)
{
	assert (NULL != shapeAllocator);
	assert (NULL != out_shape);


	if (!shape::shapeAlloc (shapeAllocator, vtxLayout, 24, 36, out_shape))
		return false;


	shape::VtxArrayWriter writer;
	writer.setup (out_shape);


	VtxArrayWriter::Elem<vec3f> vtx;
	VtxArrayWriter::Elem<vec3f> norm;
	VtxArrayWriter::Elem<vec2f> tex;
	writer.getPos3 (&vtx);
	writer.getNorm3 (&norm);
	writer.getTexCoord (&tex, 0);


	const f32 x = center.x + (size.x / 2.0f);
	const f32 y = center.y + (size.y / 2.0f);
	const f32 z = center.z + (size.z / 2.0f);

	//front
	vtx().set(-x, y,-z);	vtx.next();
	vtx().set( x, y,-z);	vtx.next();
	vtx().set( x,-y,-z);	vtx.next();
	vtx().set(-x,-y,-z);	vtx.next();
	if (writer.hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer.addTris ((nv - 4), (nv - 3), (nv - 2));
		writer.addTris ((nv - 2), (nv - 1), (nv - 4));
	}
	if (norm.isValid())
	{
		norm().set (0,0,-1);	norm.next();
		norm().set (0,0,-1);	norm.next();
		norm().set (0,0,-1);	norm.next();
		norm().set (0,0,-1);	norm.next();
	}
	if (tex.isValid())
	{
		tex().set (0,0);	tex.next();
		tex().set (1,0);	tex.next();
		tex().set (1,1);	tex.next();
		tex().set (0,1);	tex.next();
	}

	//back
	vtx().set( x, y, z);	vtx.next();
	vtx().set(-x, y, z);	vtx.next();
	vtx().set(-x,-y, z);	vtx.next();
	vtx().set( x,-y, z);	vtx.next();
	if (writer.hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer.addTris ((nv - 4), (nv - 3), (nv - 2));
		writer.addTris ((nv - 2), (nv - 1), (nv - 4));
	}
	if (norm.isValid())
	{
		norm().set (0,0,1);	norm.next();
		norm().set (0,0,1);	norm.next();
		norm().set (0,0,1);	norm.next();
		norm().set (0,0,1);	norm.next();
	}
	if (tex.isValid())
	{
		tex().set (0,0);	tex.next();
		tex().set (1,0);	tex.next();
		tex().set (1,1);	tex.next();
		tex().set (0,1);	tex.next();
	}

	//right
	vtx().set( x, y,-z);	vtx.next();
	vtx().set( x, y, z);	vtx.next();
	vtx().set( x,-y, z);	vtx.next();
	vtx().set( x,-y,-z);	vtx.next();
	if (writer.hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer.addTris ((nv - 4), (nv - 3), (nv - 2));
		writer.addTris ((nv - 2), (nv - 1), (nv - 4));
	}
	if (norm.isValid())
	{
		norm().set (1,0,0);	norm.next();
		norm().set (1,0,0);	norm.next();
		norm().set (1,0,0);	norm.next();
		norm().set (1,0,0);	norm.next();
	}
	if (tex.isValid())
	{
		tex().set (0,0);	tex.next();
		tex().set (1,0);	tex.next();
		tex().set (1,1);	tex.next();
		tex().set (0,1);	tex.next();
	}
	
	//left
	vtx().set(-x, y, z);	vtx.next();
	vtx().set(-x, y,-z);	vtx.next();
	vtx().set(-x,-y,-z);	vtx.next();
	vtx().set(-x,-y, z);	vtx.next();
	if (writer.hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer.addTris ((nv - 4), (nv - 3), (nv - 2));
		writer.addTris ((nv - 2), (nv - 1), (nv - 4));
	}
	if (norm.isValid())
	{
		norm().set (-1,0,0);	norm.next();
		norm().set (-1,0,0);	norm.next();
		norm().set (-1,0,0);	norm.next();
		norm().set (-1,0,0);	norm.next();
	}
	if (tex.isValid())
	{
		tex().set (0,0);	tex.next();
		tex().set (1,0);	tex.next();
		tex().set (1,1);	tex.next();
		tex().set (0,1);	tex.next();
	}

	//top
	vtx().set(-x, y,-z);	vtx.next();
	vtx().set(-x, y, z);	vtx.next();
	vtx().set( x, y, z);	vtx.next();
	vtx().set( x, y,-z);	vtx.next();
	if (writer.hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer.addTris ((nv - 4), (nv - 3), (nv - 2));
		writer.addTris ((nv - 2), (nv - 1), (nv - 4));
	}
	if (norm.isValid())
	{
		norm().set (0,1,0);	norm.next();
		norm().set (0,1,0);	norm.next();
		norm().set (0,1,0);	norm.next();
		norm().set (0,1,0);	norm.next();
	}
	if (tex.isValid())
	{
		tex().set (0,0);	tex.next();
		tex().set (1,0);	tex.next();
		tex().set (1,1);	tex.next();
		tex().set (0,1);	tex.next();
	}

	//bottom
	vtx().set(-x,-y,-z);	vtx.next();
	vtx().set( x,-y,-z);	vtx.next();
	vtx().set( x,-y, z);	vtx.next();
	vtx().set(-x,-y, z);	vtx.next();
	if (writer.hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer.addTris ((nv - 4), (nv - 3), (nv - 2));
		writer.addTris ((nv - 2), (nv - 1), (nv - 4));
	}
	if (norm.isValid())
	{
		norm().set (0,-1,0);	norm.next();
		norm().set (0,-1,0);	norm.next();
		norm().set (0,-1,0);	norm.next();
		norm().set (0,-1,0);	norm.next();
	}
	if (tex.isValid())
	{
		tex().set (0,0);	tex.next();
		tex().set (1,0);	tex.next();
		tex().set (1,1);	tex.next();
		tex().set (0,1);	tex.next();
	}


	assert (vtx.getcurElemNum() == out_shape->numVtx);
	assert (writer.getNumCurIndex() == out_shape->numIdx);
	return true;
}
