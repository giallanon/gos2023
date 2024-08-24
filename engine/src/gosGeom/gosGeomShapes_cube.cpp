#include "gosGeomShapes.h"
#include "../gos/gos.h"

using namespace gos;
using namespace gos::geom;


//*************************************************************
bool shape::buildCube24 (const vec3f &center, const vec3f &size, Writer *writer, Info *out_info)
{
	assert (NULL != out_info);

	out_info->numVertex = 24;
	out_info->numIndex = 36;

	//se writer e' NULL, ritorno in out le info sul num di vtx/idx necessari alla shape
	if (NULL == writer)
		return true;

	//considerando che creo 24 vtx, verifico che ci sia abb spazio nell'array di output
	if (writer->getNumMaxVertex() < 24)
	{
		logger::err ("shape::buildCube24() => not enough vertex in vtxBuffer\n");
		return false;
	}

	if (writer->hasIdxBuffer())
	{
		if (writer->getNumMaxIndex() < 36)
		{
			logger::err ("shape::buildCube24() => not enough index in idxBuffer\n");
			return false;
		}		
	}


	Writer::ElemWriter<vec3f> vtx;
	Writer::ElemWriter<vec3f> norm;
	Writer::ElemWriter<vec2f> tex;
	writer->getPos3 (&vtx);
	writer->getNorm3 (&norm);
	writer->getTexCoord (&tex, 0);


	const f32 x = center.x + (size.x / 2.0f);
	const f32 y = center.y + (size.y / 2.0f);
	const f32 z = center.z + (size.z / 2.0f);

	//front
	vtx().set(-x, y,-z);	vtx.next();
	vtx().set( x, y,-z);	vtx.next();
	vtx().set( x,-y,-z);	vtx.next();
	vtx().set(-x,-y,-z);	vtx.next();
	if (writer->hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer->addTris ((nv - 4), (nv - 3), (nv - 2));
		writer->addTris ((nv - 2), (nv - 1), (nv - 4));
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
	if (writer->hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer->addTris ((nv - 4), (nv - 3), (nv - 2));
		writer->addTris ((nv - 2), (nv - 1), (nv - 4));
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
	if (writer->hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer->addTris ((nv - 4), (nv - 3), (nv - 2));
		writer->addTris ((nv - 2), (nv - 1), (nv - 4));
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
	if (writer->hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer->addTris ((nv - 4), (nv - 3), (nv - 2));
		writer->addTris ((nv - 2), (nv - 1), (nv - 4));
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
	if (writer->hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer->addTris ((nv - 4), (nv - 3), (nv - 2));
		writer->addTris ((nv - 2), (nv - 1), (nv - 4));
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
	if (writer->hasIdxBuffer())
	{
		const u32 nv = vtx.getcurElemNum();
		writer->addTris ((nv - 4), (nv - 3), (nv - 2));
		writer->addTris ((nv - 2), (nv - 1), (nv - 4));
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

	return true;
}
