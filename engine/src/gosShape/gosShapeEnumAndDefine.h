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

	enum class eVtxLayoutFormat : u8
	{
		_1f32 = 0,
		_2f32 = 1,
		_3f32 = 2,
		_4f32 = 3,
		
		_1i32 = 4,
		_2i32 = 5,
		_3i32 = 6,
		_4i32 = 7,

		_1u32 = 8,
		_2u32 = 9,
		_3u32 = 10,
		_4u32 = 11,

		_1u8 = 12,
		_2u8 = 13,
		_3u8 = 14,
		_4u8 = 15,		

		_1i8 = 16,
		_2i8 = 17,
		_3i8 = 18,
		_4i8 = 19,		

		_1u16 = 20,
		_2u16 = 21,
		_3u16 = 22,
		_4u16 = 23,		

		_1i16 = 24,
		_2i16 = 25,
		_3i16 = 26,
		_4i16 = 27,

		unknown = 31
		//max 32 elementi
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
		static constexpr u32 NUM_MAX_ELEM = 11;

	public:
		u32		numElem;
		u32 	elemList[NUM_MAX_ELEM]; //ognuno di questi e' creato usando shape::VtxElem::define

	public:
		void 	reset()		{ numElem = 0; }
	};
	
	
	/**
	 * @brief Shape
	 */
	struct Shape
	{
	public:
		static constexpr u32 MAGIC = 0xA7320001;

	public:
		u32			magic;
		u32 		numVtx;
		u32 		numIdx;
		VtxLayout	vtxLayout;
		u8 			*vtxBuffer;
		u16 		*idxBuffer;

	public:
		void 		reset()		{ magic=Shape::MAGIC; numVtx = numIdx = 0; vtxBuffer=NULL; idxBuffer=NULL; vtxLayout.reset(); }
	};


	typedef gos::FastArray<gos::Shape> ShapeList;

 } //namespace gos

#endif //_gosShapeEnumAndDefine_h_