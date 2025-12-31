#ifndef _gosShapeEnumAndDefine_h_
#define _gosShapeEnumAndDefine_h_
#include "../gos/gosEnumAndDefine.h"
#include "../gos/gosFastArray.h"
#include "../gosGeom/gosGeomPos3.h"

namespace gos
{ 
	enum class eVtxLayoutSemantic : u8
	{
		position = 0,
		normal = 1,
		texCoord = 2,
		color = 3,
		tangent = 4,
		bitangent = 5,
		blendIndices = 6,
		blendWeight = 7,
		custom = 8
		//max 16 elementi
	};

	/**
	 * @brief VtxLayout
	 * E' una collezione di [numElem] elementi di tipo VtxElem::define (ognuno rappresentato da un u32)
	 * 
	 * Le classi di comodo per leggere/scrivere un VtxLayout sono shape::VtxLayoutReader e shape::VtxLayoutWriter
	 */
	struct VtxLayout
	{
	public:
		void 	reset()									{ numElem = 0; }

	public:
		static constexpr u32 NUM_MAX_ELEM = 11;

	public:
		u32		numElem;
		u32 	elemList[NUM_MAX_ELEM]; //ognuno di questi e' creato usando shape::VtxElem::define

	};
	
	
	/**
	 * @brief Shape
	 */
	struct Shape
	{
	public:
		u32			magic;
		u32 		numVtx;
		u32 		numIdx;
		VtxLayout	vtxLayout;
		u8 			*vtxBuffer;
		u16 		*idxBuffer;

	public:
		void 		reset()		{ magic=GOS_MAGIC__SHAPE; numVtx = numIdx = 0; vtxBuffer=NULL; idxBuffer=NULL; vtxLayout.reset(); }
	};


	typedef gos::FastArray<gos::Shape> ShapeList;

 } //namespace gos

#endif //_gosShapeEnumAndDefine_h_