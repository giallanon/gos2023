#include "gosShapePrefabs.h"
#include "gosShape.h"
#include "../gos/gos.h"

using namespace gos;
using namespace gos::shape;

//*************************************************************
static u32 shape_buildCirconferenzaXZ (const vec3f &center, f32 rx, f32 rz, u32 numPointPerCirconferenza, shape::VtxArrayWriter::Elem<vec3f> &vtx)
{
	f32 alfa = 0;
	f32 alfaINC = math::DUEPI / numPointPerCirconferenza;
	for (u32 i=0; i<numPointPerCirconferenza; i++)
	{
		const f32 x = center.x + rx * cosf(alfa);
		const f32 y = center.y;
		const f32 z = center.z + rz * sinf(alfa);
		alfa += alfaINC;

		vtx().set (x, y, z);
		vtx.next();
	}

	return numPointPerCirconferenza;
}

//*************************************************************
static void shape_buildTrisUP (u32 vtxStart, u32 vtxAlto, u32 numPointPerCirconferenza, shape::VtxArrayWriter &writer)
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
static void shape_buildTrisDOWN (u32 vtxStart, u32 vtxBasso, u32 numPointPerCirconferenza, shape::VtxArrayWriter &writer)
{
	u16 i0 = vtxStart;
	u16 i1 = vtxStart+1;
	u16 iDOWN0 = vtxBasso;
	u16 iDOWN1 = vtxBasso+1;
	for (u32 quad=0; quad<numPointPerCirconferenza; quad++)
	{
		if (quad == numPointPerCirconferenza-1)
		{
			i1 = vtxStart;
			iDOWN1 = vtxBasso;
		}
		writer.addTris (iDOWN0, i0, i1);
		writer.addTris (i1, iDOWN1, iDOWN0);

		i0++;
		iDOWN0++;
		i1++;
		iDOWN1++;
	}	
}

//*************************************************************
bool shape::buildSphere (const vec3f &center, const vec3f &radius, u32 numPointPerCirconferenza, u32 numHalfStack, const VtxLayout &vtxLayout, gos::Allocator *shapeAllocator, Shape *out_shape)
{
	assert (NULL != shapeAllocator);
	assert (NULL != out_shape);
	
	if (numHalfStack < 1)
		numHalfStack = 1;
	if (numPointPerCirconferenza<3)
		numPointPerCirconferenza = 3;

	const u32 totNumVertex = 	numPointPerCirconferenza +
								(numPointPerCirconferenza * numHalfStack) * 2 +
								2;
	
	const u32 totNumTris = (numPointPerCirconferenza * numHalfStack) * 4 + numPointPerCirconferenza*2;
	const u32 totNumIndex = totNumTris*3;


	if (!shape::shapeAlloc (shapeAllocator, vtxLayout, totNumVertex, totNumIndex, out_shape))
		return false;


	VtxArrayWriter writer;
	writer.setup (out_shape);

	VtxArrayWriter::Elem<vec3f> vtx;
	VtxArrayWriter::Elem<vec3f> norm;
	writer.getPos3 (&vtx);
	writer.getNorm3 (&norm);


	u32 nv = 0;
	f32 theta = 0;
	const f32 thetaINC = math::PIMEZZI / (numHalfStack + 1);

	//circonferenza centrale
	nv += shape_buildCirconferenzaXZ (center, radius.x, radius.z, numPointPerCirconferenza, vtx);

	//le altre circonfenze
	for (u32 nStack = 0; nStack < numHalfStack; nStack++)
	{
		theta += thetaINC;
		const f32 rx = radius.x * cosf(theta);
		const f32 rz = radius.z * cosf(theta);
		const f32 sin_theta = radius.y * sinf(theta);
		nv += shape_buildCirconferenzaXZ (vec3f(center.x, center.y + sin_theta, center.z), rx, rz, numPointPerCirconferenza, vtx);
		nv += shape_buildCirconferenzaXZ (vec3f(center.x, center.y - sin_theta, center.z), rx, rz, numPointPerCirconferenza, vtx);
	}

	//triangolarizzo
	if (writer.hasIdxBuffer())
	{
		u16 idx1 = 0;
		u16 idx2 = numPointPerCirconferenza;
		for (u32 nStack = 0; nStack < numHalfStack; nStack++)
		{
			shape_buildTrisUP (idx1, idx2, numPointPerCirconferenza, writer);
			idx1 = idx2;
			idx2 += (numPointPerCirconferenza*2);
		}

		idx1 = 0;
		idx2 = 2*numPointPerCirconferenza;
		for (u32 nStack = 0; nStack < numHalfStack; nStack++)
		{
			shape_buildTrisDOWN (idx1, idx2, numPointPerCirconferenza, writer);
			idx1 = idx2;
			idx2 += (numPointPerCirconferenza*2);
		}		
	}

	//vetice top/bottom
	vtx().set (center.x, center.y + radius.y, center.z);
	vtx.next();
	vtx().set (center.x, center.y - radius.y, center.z);
	vtx.next();
	nv += 2;

	if (writer.hasIdxBuffer())
	{
		//coperchio top
		u16 idxTOP = nv-2;
		u16 idxBase = numPointPerCirconferenza * (numHalfStack * 2 -1);
		u16 i1 = idxBase;
		for (u32 nStack = 0; nStack < numPointPerCirconferenza-1; nStack++)
		{
			writer.addTris (idxTOP, i1+1, i1);
			i1++;
		}
		writer.addTris (idxTOP, idxBase, i1);

		//coperchio bottom
		u16 idxBOTTOM = nv-1;
		idxBase += numPointPerCirconferenza;
		i1 = idxBase;
		for (u32 nStack = 0; nStack < numPointPerCirconferenza-1; nStack++)
		{
			writer.addTris (idxBOTTOM, i1, i1+1);
			i1++;
		}
		writer.addTris (idxBOTTOM, i1, idxBase);
	}

	assert(out_shape->numVtx == nv);
	assert(!writer.hasIdxBuffer()  || (writer.hasIdxBuffer() && out_shape->numIdx == writer.getNumCurIndex()));

	if (norm.isValid())
	{
		writer.getPos3 (&vtx);
		for (u32 i=0; i<out_shape->numVtx; i++)
		{
			vec3f n = vtx() - center;
			vtx.next();
			n.normalize();
			norm() = n;
			norm.next();
		}
	}
	return true;
}
