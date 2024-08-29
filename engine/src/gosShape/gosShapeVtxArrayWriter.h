#ifndef _gosShapeVtxArrayWriter_h_
#define _gosShapeVtxArrayWriter_h_
#include "gosShapeVtxLayout.h"


namespace gos
{ 
	namespace shape
	{
		/****************************
		 * VtxArrayWriter
		 * 
		 * Da usarsi per scrivere in un array di vtx precedentemente definito tramite un VtxLayout
		 * In sostanza prende un VatLayout e un buffer generico e ci scrive dentro rispettando la struttura inidicata da VtxLayout		 
		 */
		class VtxArrayWriter
		{
		public:
			template<class T>
			class Elem
			{
			public:
						Elem()								{ pt = NULL; }
				
						//operator()
						//ritorna l'elemento corrente
				T&		operator() ()
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
				u32		next ()
						{
							curVtx++;
							curOffset += sizeof_aVtx;
							return curVtx;
						}
				
						//gotoElem()
						//va all'elemento i-esimo
				void 	gotoElem (u32 iVtx)
						{
							curVtx = iVtx;
							curOffset = curVtx * sizeof_aVtx;
						}

				u32 	getcurElemNum() const			{ return curVtx; }
				bool	isValid() const 				{ return (NULL != pt); }

			protected:
				void	_setInvalid() 					{ pt = NULL; }
				void 	_setup (u8 *ptIN, u32 sizeof_aVtxIN, u32 numVtxIN)
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

				friend class VtxArrayWriter;
			};

		public:
			void	setup (const VtxLayout *vtxLayoutIN, void *vtxBufferIN, u32 sizeof_aSingleVtxIN, u32 numElemInVtxBuffer, u16 *idxBufferIN=NULL, u32 numElemInIdxBuffer=0)
					{
						vlr.setup (vtxLayoutIN);
						vtxBuffer = reinterpret_cast<u8*>(vtxBufferIN);
						sizeof_aVtx = sizeof_aSingleVtxIN;
						numVtx = numElemInVtxBuffer;
						idxBuffer = idxBufferIN;
						numIdx = numElemInIdxBuffer;
						curIdx = 0;
					}

			void	setup (Shape *shape);

					template<class T>
			bool	get (eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt, Elem<T> *out)
					{
						assert (NULL != out);
						u32 offset;
						if (vlr.find (semantic, index, fmt, &offset))
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
			VtxLayoutReader	vlr;
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

#endif //_gosShapeVtxArrayWriter_h_