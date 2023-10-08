#ifndef _gosGeomEnumAndDefine_h_
#define _gosGeomEnumAndDefine_h_
#include "GOSKernel/gosKernelEnumAndDefine.h"
#include "GOSKernel/dataTypes/gosColorU32.h"


namespace gos
{ 
	namespace geom
	{ 
		/*==================================================================
		 * channel::Def
		 *
		 *	Definisce il formato e la semantica di un singolo canale
		 */
		struct ChannelDef
		{
		public:
			u8				_index;
			u8				_semantic_and_fmt;

		public:
							ChannelDef()										{ _index = _semantic_and_fmt = 0; }
							ChannelDef(const ChannelDef &b)						{ _semantic_and_fmt=b._semantic_and_fmt; _index=b._index; }
							ChannelDef(eSemantic s, u8 i, eDataFormat ff)		{ set (s, i, ff); }

			ChannelDef&		operator= (const ChannelDef &b)						{ _semantic_and_fmt=b._semantic_and_fmt; _index=b._index; return *this; }
			bool			operator== (const ChannelDef &b) const				{ return (_semantic_and_fmt == b._semantic_and_fmt && _index==b._index); }
			bool			operator!= (const ChannelDef &b) const				{ return (_semantic_and_fmt != b._semantic_and_fmt || _index!=b._index); }
			
			void			set(eSemantic s, u8 i, eDataFormat ff)				{ _index = i; setFmt(ff); setSemantic(s); }
			void			setIndex (u8 i)										{ _index = i; }
			void			setFmt (eDataFormat ff)								{ assert((u8)ff < 16); _semantic_and_fmt &= 0xF0; _semantic_and_fmt |= (u8)ff; }
			void			setSemantic (eSemantic ff)							{ assert((u8)ff < 16); _semantic_and_fmt &= 0x0F; _semantic_and_fmt |= (((u8)ff) << 4); }

			u8				getIndex() const									{ return _index; }
			eDataFormat		getFormat() const									{ return (eDataFormat)(_semantic_and_fmt & 0x0F); }
			eSemantic		getSemantic() const									{ return (eSemantic)((_semantic_and_fmt & 0xF0) >> 4); }
			u8				getFormatSize() const;
		};


		typedef struct sShape
		{
			u8	*p;
		} Shape;

		#define SHAPE_NUM_MAX_CHANNEL	16
		typedef struct sShapeHeader
		{
			u16					signature;
			eRenderPrimitive	renderPrimitive;
			u8					numChannel;
			
			u32					totalShapeSizeInBytes;
			u32					numVertex;
			u32					numIndex;

			u32					offsetToIndexBuffer;
			u32					offsetToChannelData[SHAPE_NUM_MAX_CHANNEL];

			ChannelDef			chDef[SHAPE_NUM_MAX_CHANNEL];
		} ShapeHeader;


	} //namespace geom
 } //namespace gos

#endif //_gosGeomEnumAndDefine_h_