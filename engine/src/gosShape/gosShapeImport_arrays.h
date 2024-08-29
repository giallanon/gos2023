#ifndef _gosShapeImport_arrays_h_
#define _gosShapeImport_arrays_h_
#include "gosShape.h"
#include "gosShapeVtxArrayWriter.h"
#include "../gos/gosFastArray.h"


namespace gos
{ 
	namespace shape
	{
		class ArraysImporter
		{
		public:
					ArraysImporter();
					~ArraysImporter();

			bool 	create (const VtxLayout &desiredLayout, gos::Allocator *allocator, Shape *out,
							const FastArray<u16> *trisList, u8 numIdxPerOgniVtxDiTrisList,
							const FastArray<vec3f> *posList, u8 indexOffsetForPosition,
							const FastArray<vec3f> *normList, u8 indexOffsetForNormal,
							const FastArray<vec2f> *tutv0List, u8 indexOffsetForTutv0);

		private:
			static constexpr u32 NUM_MAX_INDEX_PER_TUPLA = 16;

		private:
			struct sTupla
			{
				u16	idx[NUM_MAX_INDEX_PER_TUPLA];
			};

			struct sVertex
			{
				u8 	numIndices;
				u16	idx[NUM_MAX_INDEX_PER_TUPLA];

				bool 	operator== (const sVertex &b) const
						{
							if (numIndices != b.numIndices) return false;
							for (u8 i=0; i<numIndices; i++)
							{
								if (idx[i] != b.idx[i])
									return false;
							}
							return true;
						}
			};

			template <class T>
			class DataArray
			{
			public:
						DataArray()																		{ setInvalid(); }
				
				void 	setup (const FastArray<T> *listIN, u8 offsetInTuplaIN, u8 offsetInVtxIN)		{ list = listIN; offsetInTupla = offsetInTuplaIN; offsetInVtx = offsetInVtxIN; }
				void 	setInvalid()																	{ list = NULL; offsetInTupla=offsetInVtx=0xFF; }
				bool 	isValid() const 																{ return (offsetInTupla!=0xFF); }

			public:
				const FastArray<T> 	*list;
				u8					offsetInTupla;
				u8					offsetInVtx;
			};


			struct sImportData
			{
				DataArray<vec3f>	position;
				DataArray<vec3f>	norm;
				DataArray<vec2f>	tutv0;
			};


		private:
			bool	priv_extractTupla (const FastArray<u16> *trisList, u32 iStart, u32 numIdxPerTupla, sTupla *out) const;
			void 	priv_extractVertex (const sTupla &tupla, const sImportData &importData, sVertex *out) const;
			u32 	priv_findOrCreateVtx (const sVertex &vIN, const sImportData &importData);

					template<class T>
			void 	priv_finalizeShapeVtxBuffer (VtxArrayWriter &writer, const sImportData &importData, const DataArray<T> &dataArray, eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt)
			{
				if (dataArray.isValid())
				{
					VtxArrayWriter::Elem<T>	elem;
					writer.get (semantic, index, fmt, &elem);

					const u32 offset = dataArray.offsetInVtx;
					const FastArray<T> 	*source = dataArray.list;
					for (u32 i=0; i<finalVtxList.getNElem(); i++)
					{
						const u32 idx = finalVtxList(i).idx[offset];
						elem() = source->queryElem(idx);
						elem.next();				
					}
				}
			}
		private:
			FastArray<sVertex>	finalVtxList;
			FastArray<u16>		finalIdxList;
		};
		
	} //namespace shape
 } //namespace gos

#endif //_gosShapeImport_arrays_h_