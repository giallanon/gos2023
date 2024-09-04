#ifndef _gosShapeVtxArrayReader_h_
#define _gosShapeVtxArrayReader_h_
#include "gosShapeVtxLayout.h"


namespace gos
{ 
	namespace shape
	{
		/**
		 * @brief 	VtxArrayReader
		 * 	 		Da usarsi per leggere in un generico buffer che si suppone essere formattato secondo la descrizione fornita
		 * 			da un VtxLayout
		 */
		class VtxArrayReader
		{
		public:
			template<class T>
			class Elem
			{
			public:
							Elem()								{ pt = NULL; }
				
							//operator()
							//ritorna l'elemento corrente
				const T&	operator() ()
							{ 
								const T* ret;
								if (curVtx < numVtx)	
									ret = reinterpret_cast<const T*>(&pt[curOffset]); 
								else
								{
									DBGBREAK;
									ret = reinterpret_cast<const T*>(pt); 
								}
								return *ret;
							}
				
							//next()
							//avanza al prossimo elemento
				u32			next ()								{ curVtx++; curOffset += sizeof_aVtx; return curVtx; }
				
							//gotoElem()
							//va all'elemento i-esimo
				void 		gotoElem (u32 iVtx)					{ curVtx = iVtx; curOffset = curVtx * sizeof_aVtx; }

				u32 		getcurElemNum() const				{ return curVtx; }
				bool		isValid() const 					{ return (NULL != pt); }

			protected:
				void		_setInvalid() 					{ pt = NULL; }
				void 		_setup (const u8 *ptIN, u32 sizeof_aVtxIN, u32 numVtxIN)
							{
								pt = ptIN;
								sizeof_aVtx = sizeof_aVtxIN;
								numVtx = numVtxIN;
								curOffset = 0;
								curVtx = 0;
							}

			private:
				const u8 	*pt;
				u32 		sizeof_aVtx;
				u32 		numVtx;
				u32 		curVtx;
				u32 		curOffset;

				friend class VtxArrayReader;
			};

		public:
			void	setup (const VtxLayout *vtxLayoutIN, const void *vtxBufferIN, u32 sizeof_aSingleVtxIN, u32 numElemInVtxBuffer)
					{
						vlr.setup (vtxLayoutIN);
						vtxBuffer = reinterpret_cast<const u8*>(vtxBufferIN);
						sizeof_aVtx = sizeof_aSingleVtxIN;
						numVtx = numElemInVtxBuffer;
					}

			void	setup (const Shape *shape);

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
			bool 	getTan3 (Elem<vec3f> *out)							{ return get<vec3f> (eVtxLayoutSemantic::tangent, 0, eVtxLayoutFormat::_3f32, out); }
			bool 	getBitan3 (Elem<vec3f> *out)						{ return get<vec3f> (eVtxLayoutSemantic::bitangent, 0, eVtxLayoutFormat::_3f32, out); }

			u32 	getNumMaxVertex() const 							{ return numVtx; }

		private:
			VtxLayoutReader	vlr;
			const u8		*vtxBuffer;
			u32 			sizeof_aVtx;
			u32 			numVtx;
		};


	} //namespace shape
 } //namespace gos

#endif //_gosShapeVtxArrayReader_h_