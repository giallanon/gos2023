#ifndef _gosShapeVtxLayout_h_
#define _gosShapeVtxLayout_h_
#include "gosShapeVtxElem.h"


namespace gos
{ 
	namespace shape
	{
		/***************************************
		 * VtxLayoutInterface
		 * 
		 * template di comodo per la lettura di un VtxLayout
		 */
		template <class VTXLAYOUT>
		class VtxLayoutReaderInterface
		{
		public:
								VtxLayoutReaderInterface ()																		{ }
								VtxLayoutReaderInterface (VTXLAYOUT v)																	{ setup(v); }
								~VtxLayoutReaderInterface ()																	{ }

			void 				setup (VTXLAYOUT v)																						{ vl = v; }

			bool				find (eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt, u32 *out_offset) const		{ assert (NULL != out_offset); *out_offset = priv_findOffset(semantic, index, fmt); return (*out_offset != u32MAX); }
			bool 				exists  (eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt) const						{ return (priv_findOffset(semantic, index, fmt) != u32MAX); }

			u32 				getNumElem () const 																			{ return vl->numElem; }
			u32 				getOffset (u32 elemNum)	const																	{ assert(elemNum<getNumElem()); return VtxElem::getOffset(vl->elemList[elemNum]); }
			eVtxLayoutSemantic	getSemantic (u32 elemNum) const																	{ assert(elemNum<getNumElem()); return VtxElem::getSemantic(vl->elemList[elemNum]); }
			u8					getIndex (u32 elemNum) const																	{ assert(elemNum<getNumElem()); return VtxElem::getIndex(vl->elemList[elemNum]); }
			eVtxLayoutFormat	getFormat (u32 elemNum)	const																	{ assert(elemNum<getNumElem()); return VtxElem::getFormat(vl->elemList[elemNum]); }

		protected:
			VTXLAYOUT			vl;

		private:
			u32					priv_findOffset (eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt) const
								{
									u32 ret;
									const u32 key = VtxElem::buildSearchKey (semantic, index, fmt);
									for (u32 i=0; i<getNumElem(); i++)
									{
										if (VtxElem::doesKeyMatch (vl->elemList[i], key, &ret))
											return ret;
									}
									return u32MAX;
								}

		};	


		/*********************************************
		 * VtxLayoutReader
		 * 
		 * Da usarsi come classe di comodo per accedere in sola lettura alle
		 * info di VtxLayer 
		*/
		class VtxLayoutReader : public VtxLayoutReaderInterface<const VtxLayout*>
		{
		public:
					VtxLayoutReader ()																			{ }
					VtxLayoutReader (const VtxLayout *v) : VtxLayoutReaderInterface<const VtxLayout*>(v) 		{ }
					~VtxLayoutReader ()																			{ }
		};

		/***************************************
		 * VtxLayoutWriter
		 * 
		 * Da usarsi come classe di comodo per accedere in lettura/scrittura alle
		 * info di VtxLayer 
		 */
		class VtxLayoutWriter : public VtxLayoutReaderInterface<VtxLayout*>
		{
		public:
								VtxLayoutWriter()																		{ }
								VtxLayoutWriter(VtxLayout *v) : VtxLayoutReaderInterface<VtxLayout*>(v) 				{ setup(v); }
								~VtxLayoutWriter()																		{ }

			VtxLayoutWriter& 	begin ()																				{ vl->numElem = 0; return *this; }
			VtxLayoutWriter&		add (u32 offset, eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt)
									{
										if (getNumElem() >= VtxLayout::NUM_MAX_ELEM)
										{
											vl->numElem = 0xff;
											return *this;
										}
										vl->elemList[vl->numElem++] = VtxElem::define (offset, semantic, index, fmt);
										return *this;
									}

			VtxLayoutWriter&		addPos3 (u32 offset, u8 index=0) 													{ return add (offset, eVtxLayoutSemantic::position, index, eVtxLayoutFormat::_3f32); }
			VtxLayoutWriter&		addNorm3 (u32 offset, u8 index=0) 													{ return add (offset, eVtxLayoutSemantic::normal, index, eVtxLayoutFormat::_3f32); }
			VtxLayoutWriter&		addTexCoord (u32 offset, u8 index=0) 												{ return add (offset, eVtxLayoutSemantic::texCoord, index, eVtxLayoutFormat::_2f32); }
			VtxLayoutWriter&		addColor3 (u32 offset, u8 index=0)													{ return add (offset, eVtxLayoutSemantic::color, index, eVtxLayoutFormat::_3f32); }
			bool				end();

		};
	} //namespace shape
 } //namespace gos

#endif //_gosShapeVtxLayout_h_