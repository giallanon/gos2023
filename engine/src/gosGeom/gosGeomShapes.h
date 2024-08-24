#ifndef _gosGeomShapes_h_
#define _gosGeomShapes_h_
#include "gosGeomEnumAndDefine.h"
#include "gosGeomPos3.h"

namespace gos
{ 
	namespace shape
	{
		/**************************************
		 * VtxMap
		 * 
		 * Mappa un generico layout di vertex
		 * 
		 */
		class VtxMap
		{
		public:


		public:
			VtxMap& 	begin ();
			VtxMap&		add (u32 offset, eVtxMapSemantic semantic, u8 index, eVtxMapType type, u8 numElem);
			VtxMap&		addPos3 (u32 offset, u8 index=0) 				{ return add (offset, eVtxMapSemantic::position, index, eVtxMapType::f32, 3); }
			VtxMap&		addNorm3 (u32 offset, u8 index=0) 				{ return add (offset, eVtxMapSemantic::normal, index, eVtxMapType::f32, 3); }
			VtxMap&		addTexCoord (u32 offset, u8 index=0) 			{ return add (offset, eVtxMapSemantic::texCoord, index, eVtxMapType::f32, 2); }
			bool		end();

			bool		find (eVtxMapSemantic semantic, u8 index, eVtxMapType type, u8 numElem, u32 *out_offset) const;

		private:
			static const u8 NUM_MAX_ELEM = 8;
		
		private:
			u32 	elemList[NUM_MAX_ELEM];
			u8		n;
		};

		/****************************
		 * Writer
		 * 
		 * Da usarsi per scrivere in un array di vtx precedentemente
		 * mappato da una VtxMap
		 */
		class Writer
		{
		public:
			template<class T>
			class ElemWriter
			{
			public:
							ElemWriter()					{ pt = NULL; }
				
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
				u32			next ()
							{
								curVtx++;
								curOffset += sizeof_aVtx;
								return curVtx;
							}
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

				friend class Writer;
			};

		public:
			void	setup (const VtxMap &vtxMapIN, void *vtxBufferIN, u32 sizeof_aSingleVtxIN, u32 numElemInVtxBuffer, u16 *idxBufferIN=NULL, u32 numElemInIdxBuffer=0)
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
			bool	get (eVtxMapSemantic semantic, u8 index, eVtxMapType type, u8 numElem, ElemWriter<T> *out)
					{
						assert (NULL != out);
						u32 offset;
						if (vtxMap.find (semantic, index, type, numElem, &offset))
							out->_setup (&vtxBuffer[offset], sizeof_aVtx, numVtx);
						else
							out->_setInvalid();
						return out->isValid();
					}

			bool 	getPos3 (ElemWriter<vec3f> *out)					{ return get<vec3f> (eVtxMapSemantic::position, 0, eVtxMapType::f32, 3, out); }
			bool 	getNorm3 (ElemWriter<vec3f> *out)					{ return get<vec3f> (eVtxMapSemantic::normal, 0, eVtxMapType::f32, 3, out); }
			bool 	getTexCoord (ElemWriter<vec2f> *out, u8 index=0)	{ return get<vec2f> (eVtxMapSemantic::texCoord, index, eVtxMapType::f32, 2, out); }

			bool	hasIdxBuffer() const 								{ return (NULL != idxBuffer); }
			void 	addTris (u16 i0, u16 i1, u16 i2)					{ assert(curIdx<=numIdx-3);idxBuffer[curIdx++]=i0; idxBuffer[curIdx++]=i1; idxBuffer[curIdx++]=i2; }
			void 	setIndex (u32 i, u16 val)							{ assert(i<numIdx); idxBuffer[i] = val; }
			void 	gotoIndex (u32 i)									{ curIdx = i; }

			u32 	getNumMaxVertex() const 							{ return numVtx; }
			u32 	getNumMaxIndex() const 								{ return numIdx; }
			u32 	getNumCurIndex() const 								{ return curIdx; }

		private:
			VtxMap 	vtxMap;
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

		//buildCube24()
		// Crea una cubo con 4 vtx separati per ogni faccia (24 vtx in totale) e 6 idx per faccia (36idx in totale)
		// In base a come e' statp definito [writer], filla "pos", "norm" e "texCoord"
		// Se [writer] == NULL, filla [out_info] con il num di vtx/idx necessari
		bool 	buildCube24 (const vec3f &center, const vec3f &size, Writer *writer, Info *out_info);

		//buildSphere()
		//Crea la circonferenza di base attorno a [center] e poi [numHalfStack] circonferenze verso l'altro e altrettante verso il basso
		// Se [writer] == NULL, filla [out_info] con il num di vtx/idx necessari
		bool 	buildSphere (const vec3f &center, const vec3f &radius, u32 numPointPerCirconferenza, u32 numHalfStack, Writer *writer, Info *out_info);
		
		//buildCylinder
		//La base e' centrata in [center] e sale verso l'alto. Crea [numStack] circonferenze (compresa quella di base)
		// Se [writer] == NULL, filla [out_info] con il num di vtx/idx necessari
		bool 	buildCylinder (const vec3f &center, f32 radius, f32 height, u32 numPointPerCirconferenza, u32 numStack, bool bCloseTop, bool bCloseBottom, Writer *writer, Info *out_info);

	} //namespace shape
 } //namespace gos

#endif //_gosGeomShapes_h_