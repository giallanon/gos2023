#include "gosShapePrefabs.h"
#include "gosShape.h"
#include "../gos/gos.h"

using namespace gos;
using namespace gos::shape;

//*************************************************************
u32 shape_buildCirconferenzaXZ (const vec3f &center, f32 radius, u32 numPointPerCirconferenza, shape::VtxArrayWriter::Elem<vec3f> &vtx)
{
	f32 alfa = 0;
	f32 alfaINC = math::DUEPI / numPointPerCirconferenza;
	for (u32 i=0; i<numPointPerCirconferenza; i++)
	{
		const f32 x = center.x + radius * cosf(alfa);
		const f32 y = center.y;
		const f32 z = center.z + radius * sinf(alfa);
		alfa += alfaINC;

		vtx().set (x, y, z);
		vtx.next();
	}

	return numPointPerCirconferenza;
}

//*************************************************************
void shape_buildTrisUP (u32 vtxStart, u32 vtxAlto, u32 numPointPerCirconferenza, shape::VtxArrayWriter &writer)
{
	u16 i0 = vtxStart;
	u16 i1 = i0+1;
	u16 iUP0 = vtxAlto;
	u16 iUP1 = iUP0+1;
	for (u32 quad=0; quad<numPointPerCirconferenza; quad++)
	{
		if (quad == numPointPerCirconferenza-1)
		{
			i1 = vtxStart;
			iUP1 = vtxAlto;
		}
		writer.addTris (i0, iUP0, iUP1);
		writer.addTris (iUP1, i1, i0);

		i0++;
		iUP0++;
		i1++;
		iUP1++;
	}	
}

//*************************************************************
bool shape::buildCylinder (const vec3f &center, f32 radius, f32 heightIN, u32 numPointPerCirconferenza, u32 numStack, bool bCloseTop, bool bCloseBottom, gos::Allocator *allocator, Shape *shapeIN)
{
	assert (NULL != shapeIN);
	assert (NULL != allocator);
	
	if (numStack < 2)
		numStack = 2;
	if (numPointPerCirconferenza<3)
		numPointPerCirconferenza = 3;

	u32 totNumVertex = numPointPerCirconferenza * numStack;
	if (bCloseTop)
		totNumVertex++;
	if (bCloseBottom)
		totNumVertex++;
	
	u32 totNumTris = (numPointPerCirconferenza * (numStack-1)) * 2;
	if (bCloseTop)
		totNumTris += numPointPerCirconferenza;
	if (bCloseBottom)
		totNumTris += numPointPerCirconferenza;
	const u32 totNumIndex = totNumTris*3;


	if (!shape::shapeAlloc (allocator, totNumVertex, totNumIndex, shapeIN))
		return false;
	
	VtxArrayWriter writer;
	writer.setup (shapeIN);

	VtxArrayWriter::Elem<vec3f> vtx;
	VtxArrayWriter::Elem<vec3f> norm;
	writer.getPos3 (&vtx);
	writer.getNorm3 (&norm);


	u32 nv = 0;
	f32 height = 0;
	const f32 heightINC = heightIN / (numStack-1);

	//circonfenze
	for (u32 nStack = 0; nStack < numStack; nStack++)
	{
		nv += shape_buildCirconferenzaXZ (vec3f(center.x, center.y + height, center.z), radius, numPointPerCirconferenza, vtx);
		height += heightINC;
	}

	//triangolarizzo
	if (writer.hasIdxBuffer())
	{
		u16 idx1 = 0;
		u16 idx2 = numPointPerCirconferenza;
		for (u32 nStack = 0; nStack < (numStack-1); nStack++)
		{
			shape_buildTrisUP (idx1, idx2, numPointPerCirconferenza, writer);
			idx1 = idx2;
			idx2 += numPointPerCirconferenza;
		}
	}

	//vetice top/bottom
	if (bCloseTop)
	{
		vtx().set (center.x, center.y + heightIN, center.z);
		vtx.next();
		nv++;

		if (writer.hasIdxBuffer())
		{
			//coperchio top
			u16 idxTOP = nv-1;
			u16 idxBase = nv -1 - numPointPerCirconferenza;
			u16 i1 = idxBase;
			for (u32 i = 0; i < numPointPerCirconferenza-1; i++)
			{
				writer.addTris (idxTOP, i1+1, i1);
				i1++;
			}
			writer.addTris (idxTOP, idxBase, i1);
		}
	}

	if (bCloseBottom)
	{
		vtx().set (center.x, center.y, center.z);
		vtx.next();
		nv++;
		if (writer.hasIdxBuffer())
		{
			u16 idxBOTTOM = nv-1;
			u16 idxBase = 0;
			u16 i1 = idxBase;
			for (u32 i = 0; i < numPointPerCirconferenza-1; i++)
			{
				writer.addTris (idxBOTTOM, i1, i1+1);
				i1++;
			}
			writer.addTris (idxBOTTOM, i1, idxBase);
		}
	}


	assert(shapeIN->numVtx == nv);
	assert(!writer.hasIdxBuffer()  || (writer.hasIdxBuffer() && shapeIN->numIdx == writer.getNumCurIndex()));

	if (norm.isValid())
	{
		writer.getPos3 (&vtx);
		for (u32 i = 0; i < numPointPerCirconferenza; i++)
		{
			vec3f n = vtx() - center;
			vtx.next();
			n.normalize();
			norm() = n;
			norm.next();
		}

		for (u32 nStack = 1; nStack < numStack; nStack++)
		{
			VtxArrayWriter::Elem<vec3f> normBase;
			writer.getPos3 (&normBase);
			for (u32 i = 0; i < numPointPerCirconferenza; i++)
			{
				norm() = normBase();
				norm.next();
				normBase.next();
			}
		}
		
		if (bCloseTop)
		{
			norm().set (0,1,0);
			norm.next();
		}

		if (bCloseBottom)
		{
			norm().set (0,-1,0);
			norm.next();
		}			
	}
	return true;
}
