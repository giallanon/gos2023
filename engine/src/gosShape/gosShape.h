#ifndef _gosShape_h_
#define _gosShape_h_
#include "gosShapeEnumAndDefine.h"
#include "../gosGeom/gosGeomPos3.h"

namespace gos
{ 
	namespace shape
	{
		namespace VtxElem
		{
			u32 					define (u32 offset, eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt);
			u32 					getOffset (u32 elem);
			eVtxLayoutSemantic		getSemantic(u32 elem);
			u8						getIndex(u32 elem);
			eVtxLayoutFormat		getFormat (u32 elem);
			
			u32 					buildSearchKey (eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt);
			bool 					doesKeyMatch (u32 elem, u32 key, u32 *out_offset);
		} //namespace VtxElem

		/***************************************
		 * VtxLayout
		 */
		class VtxLayout
		{
		public:
								VtxLayout()																				{ numElem=0; }

			bool				operator == (const VtxLayout &b)														{ return priv_areEqual(b); }
			bool				operator != (const VtxLayout &b)														{ return !priv_areEqual(b); }

			VtxLayout& 			begin ()																				{ numElem = 0; return *this; }
			VtxLayout&				add (u32 offset, eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt)
									{
										if (numElem >= NUM_MAX_ELEM)
										{
											numElem = 0xff;
											return *this;
										}
										elemList[numElem++] = VtxElem::define (offset, semantic, index, fmt);
										return *this;
									}

			VtxLayout&				addPos3 (u32 offset, u8 index=0) 													{ return add (offset, eVtxLayoutSemantic::position, index, eVtxLayoutFormat::_3f32); }
			VtxLayout&				addNorm3 (u32 offset, u8 index=0) 													{ return add (offset, eVtxLayoutSemantic::normal, index, eVtxLayoutFormat::_3f32); }
			VtxLayout&				addTexCoord (u32 offset, u8 index=0) 												{ return add (offset, eVtxLayoutSemantic::texCoord, index, eVtxLayoutFormat::_2f32); }
			bool				end();
	
			bool				find (eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt, u32 *out_offset) const;

			u32 				getNumElem() const 				{ return numElem; }
			u32 				getOffset (u32 elemNum)	const	{ assert(elemNum<getNumElem()); return VtxElem::getOffset(elemList[elemNum]); }
			eVtxLayoutSemantic	getSemantic(u32 elemNum) const	{ assert(elemNum<getNumElem()); return VtxElem::getSemantic(elemList[elemNum]); }
			u8					getIndex(u32 elemNum) const		{ assert(elemNum<getNumElem()); return VtxElem::getIndex(elemList[elemNum]); }
			eVtxLayoutFormat	getFormat (u32 elemNum)	const	{ assert(elemNum<getNumElem()); return VtxElem::getFormat(elemList[elemNum]); }

			u32 				serialize (u8 *buffer, u32 sizeof_buffer) const;
			u32 				deserialize (const u8 *buffer, u32 sizeof_buffer);

		private:
			static constexpr u32 NUM_MAX_ELEM = 11;

		private:
			bool				priv_areEqual (const VtxLayout &b) const;
		private:
			u32	numElem;
			u32 elemList[NUM_MAX_ELEM];
		};


		/****************************
		 * VtxWriter
		 * 
		 * Da usarsi per scrivere in un array di vtx precedentemente
		 * mappato da una VtxLayout
		 */
		class VtxWriter
		{
		public:
			template<class T>
			class Elem
			{
			public:
						Elem()								{ pt = NULL; }
				
						//operator()
						//ritorna l'elemento corrente
				T&			operator() ()
							{ 
								T* ret;
								if (curVtx < numVtx)	
									ret = reinterpret_cast<T*>(&pt[curOffset]); 
								else
								{
									DBGBREAK;
									ret = reinterpret_cast<T*>(pt); 
								}
								return *ret;
							}
				
						//next()
						//avanza al prossimo elemento
				u32			next ()
							{
								curVtx++;
								curOffset += sizeof_aVtx;
								return curVtx;
							}
				
						//gotoElem()
						//va all'elemento i-esimo
				void 		gotoElem (u32 iVtx)
							{
								curVtx = iVtx;
								curOffset = curVtx * sizeof_aVtx;
							}

				u32 		getcurElemNum() const			{ return curVtx; }
				bool		isValid() const 				{ return (NULL != pt); }

			protected:
				void		_setInvalid() 					{ pt = NULL; }
				void 		_setup (u8 *ptIN, u32 sizeof_aVtxIN, u32 numVtxIN)
							{
								pt = ptIN;
								sizeof_aVtx = sizeof_aVtxIN;
								numVtx = numVtxIN;
								curOffset = 0;
								curVtx = 0;
							}

			private:
				u8 		*pt;
				u32 	sizeof_aVtx;
				u32 	numVtx;
				u32 	curVtx;
				u32 	curOffset;

				friend class VtxWriter;
			};

		public:
			void	setup (const VtxLayout &vtxMapIN, void *vtxBufferIN, u32 sizeof_aSingleVtxIN, u32 numElemInVtxBuffer, u16 *idxBufferIN=NULL, u32 numElemInIdxBuffer=0)
					{
						vtxMap = vtxMapIN;
						vtxBuffer = reinterpret_cast<u8*>(vtxBufferIN);
						sizeof_aVtx = sizeof_aSingleVtxIN;
						numVtx = numElemInVtxBuffer;
						idxBuffer = idxBufferIN;
						numIdx = numElemInIdxBuffer;
						curIdx = 0;
					}

					template<class T>
			bool	get (eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt, Elem<T> *out)
					{
						assert (NULL != out);
						u32 offset;
						if (vtxMap.find (semantic, index, fmt, &offset))
							out->_setup (&vtxBuffer[offset], sizeof_aVtx, numVtx);
						else
							out->_setInvalid();
						return out->isValid();
					}

			bool 	getPos3 (Elem<vec3f> *out)							{ return get<vec3f> (eVtxLayoutSemantic::position, 0, eVtxLayoutFormat::_3f32, out); }
			bool 	getNorm3 (Elem<vec3f> *out)							{ return get<vec3f> (eVtxLayoutSemantic::normal, 0, eVtxLayoutFormat::_3f32, out); }
			bool 	getTexCoord (Elem<vec2f> *out, u8 index=0)			{ return get<vec2f> (eVtxLayoutSemantic::texCoord, index, eVtxLayoutFormat::_2f32, out); }

			bool	hasIdxBuffer() const 								{ return (NULL != idxBuffer); }
			void 	addTris (u16 i0, u16 i1, u16 i2)					{ assert(curIdx<=numIdx-3);idxBuffer[curIdx++]=i0; idxBuffer[curIdx++]=i1; idxBuffer[curIdx++]=i2; }
			void 	setIndex (u32 i, u16 val)							{ assert(i<numIdx); idxBuffer[i] = val; }
			void 	gotoIndex (u32 i)									{ curIdx = i; }

			u32 	getNumMaxVertex() const 							{ return numVtx; }
			u32 	getNumMaxIndex() const 								{ return numIdx; }
			u32 	getNumCurIndex() const 								{ return curIdx; }

		private:
			VtxLayout 	vtxMap;
			u8		*vtxBuffer;
			u16		*idxBuffer;
			u32 	sizeof_aVtx;
			u32 	numVtx;
			u32 	numIdx;
			u32 	curIdx;
		};


		struct Info
		{
			u32	numVertex;
			u32 numIndex;
		};

	} //namespace shape
 } //namespace gos

#endif //_gosShape_h_