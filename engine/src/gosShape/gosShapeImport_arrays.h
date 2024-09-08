#ifndef _gosShapeImport_arrays_h_
#define _gosShapeImport_arrays_h_
#include "gosShape.h"
#include "gosShapeVtxArrayWriter.h"
#include "../gos/gosFastArray.h"
#include "../gos/gosHashMap.h"


namespace gos
{ 
	namespace shape
	{
		/**
		 * @brief crea una Shape a partire da un set di liste di posizioni, normali, texture coord...
		 * 
		 * @fn beginUsingFaceList()
		 * Il fulcro dell'algoritmo si basa tu @tupleList che contiene i vertici dei triangoli da creare.
		 * Ogni vertice e' composto da @numIdxPerOgniTupla u16 che a loro volta indicizzano i vari array di pos, norm etc.
		 * Ad esempio, se abbiamo un @tupleList con @numIdxPerOgniTupla=3, e abbiamo 2 triangoli, ci aspettiamo di avere un totale
		 * di 6 tuple in @tupleList (ovvero 6 vertici) ognuno compostao da 3 indici, per un totale di 6*3=18 indici in tutto (@tupleList quindi deve contenere 
		 * 18 indici).
		 * Prendiamo la prima, la seconda e la terza tupla per avere i 3 vtx del primo triangolo.
		 * Una tupla in questo caso e' composta da 3 indici e potrebbe essere una cosa del tipo (10, 3, 14).
		 * Supponendo che l'ordine degli indici sia posizione, normale, texCoord0, allora questo vertice e' composto da posizione[10], normale[3], texCoord[14]
		 * 
		 * Per conoscere l'ordine degli indici, si usa @offsetInTupla dell'array aggiunto con addImportArray()
		 * In questo caso quindi, per indicare che il primo elemento della tupla e' la posizione, il secondo e' la normale e il terzo e' texCoord, bisognera' chiamare
		 * 		addImportArray (list, 0, eVtxLayoutSemantic::position)
		 *  	addImportArray (list, 1, eVtxLayoutSemantic::normal)
		 *  	addImportArray (list, 2, eVtxLayoutSemantic::texCoord)
		 * 
		 * Ogni volta che si chiama addImportArray(), viene verificato che il rispettivo semantica-semanticaIndex-fmt esista nel VtxLayout desiderato.
		 * 
		 * Questo ragionametno deriva dai formati tipo OBJ e DAE che riportano un insieme di posizioni, normali e texCoord e poi un indice delle faccie
		 * che fanno riferimento a questo set di posizioni/normali/coord.
		 * 
		 * 		 * 
		 * @fn beginUsingRealIdxBuffer
		 * funziona come beginUsingFaceList() solo che in input prende un vero idx buffer e quindi e' molto pi' performante.
		 * Un index buffer con i valor (7, 13, 18) indica un tris i cui vertici prendono pos/norm/texCoord dai rispettivi canali all'indirizzo 7 (pos[7], norm[7], tutv[7]),
		 * 13 e 18
		 * 
		 * 
		 * @param[in] desiredLayout indica il VtxLayout della shape che si vuole costruire
		 * 
		 * @param[in] tupleList e' un elenco di tuple, ciascuna composta da @numIdxPerOgniTupla indici. Ogni tupla indica un vertice. 3 tuple indicano un triangolo
		 * 
		 * @param[in] numIdxPerOgniTupla indica il numero di indici di ogni tupla di @tupleList
		 * 
		 * @param[in] list e' un elenco che posizioni, o normali, o texCoord, o tangenti...
		 * 
		 * @param[in] offsetInTupla indica la posizione all'interno di una tupla (elemento 0, elemento 1, elemento 2...)
		 * 
		 * @param[in] shapeAllocator indica l'allocator da utilizzare per creare la shape
		 * 
		 * @param[out] out_shape se la fn ritorna true, qui c'e' la shape che e' stata creata
		 */
		class ArraysImporter
		{
		public:
					ArraysImporter();
					~ArraysImporter();

			ArraysImporter&	beginUsingFaceList (const VtxLayout &desiredLayout, const u16 *tupleList, u32 totNumOfIdxInTupleList, u8 numIdxPerOgniTupla);
			ArraysImporter&	beginUsingRealIdxBuffer (const VtxLayout &desiredLayout, const u16 *idxBuffer, u32 totNumOfIdxInIdxBuffer);
			
							template<class T>
			ArraysImporter&	addImportArray (const void *listIN, u8 offsetInTupla, eVtxLayoutSemantic semanticIN, u8 semanticIndexIN, eVtxLayoutFormat fmt)
							{
								//se stiamo usando un vero idxBuffer, l'offset di quelo elemeno nell'idx buffer non serve
								if (eMode::importFromRealIdxBuffer == mode)
									offsetInTupla = 0;

								const T *list = reinterpret_cast<const T*>(listIN);
								shape::VtxLayoutReader vxtLayoutR(&shapeVtxLayout);

								//verifico che (semantic, semanticIdx, fmt) sia compatibile con il vtxLayout indicato durante begin()
								if (!vxtLayoutR.exists (semanticIN, semanticIndexIN, fmt))
								{
									gos::logger::verbose ("shape::ArraysImporter::addImportArray() => VtxLayout does not contains '%s(%d) %s'\n", enumToString(semanticIN), semanticIndexIN, enumToString(fmt));
									errorCode = 1;
									return *this;
								}
								if (offsetInTupla >= numIdxPerOgniTupla)
								{
									errorCode = 1;
									gos::logger::verbose ("shape::ArraysImporter::addImportArray() => 'offsetInTupla'' for '%s(%d) %s' is out of range\n", enumToString(semanticIN), semanticIndexIN, enumToString(fmt));
									return *this;
								}

								//aggiungo il canale di import
								importData.addChannel<T> (semanticIN, semanticIndexIN, fmt, list, offsetInTupla, nextValidOffsetInVtx++);	
								return *this;
							}

			bool 			end (gos::Allocator *shapeAllocator, Shape *out_shape);

			u8 				getErrorCode() const 													{ return errorCode; }

		private:
			static constexpr u32 NUM_MAX_INDEX_PER_TUPLA = 8;

		private:
			enum class eMode : u8
			{
				importFromFaceList = 0,
				importFromRealIdxBuffer = 1
			};

		private:
			struct sTupla
			{
				u16	idx[NUM_MAX_INDEX_PER_TUPLA];
			};

			struct sVertex
			{
				void 	reset ()									{ numIndices = 0; data.asU64.idx[0] = data.asU64.idx[1] = u64MAX; }
				void 	add (u32 position, u32 index)				
				{ 
					assert(index < 0xFFFF); 
					assert(position < NUM_MAX_INDEX_PER_TUPLA); 
					numIndices++; 
					data.asU16.idx[position] = static_cast<u16>(index); 
				}
				
				bool 	operator== (const sVertex &b) const
						{
							return (data.asU64.idx[0] == b.data.asU64.idx[0]  && data.asU64.idx[1] == b.data.asU64.idx[1]);
							/*if (numIndices != b.numIndices) return false;
							for (u8 i=0; i<numIndices; i++)
							{
								if (data.asU16.idx[i] != b.data.asU16.idx[i])
									return false;
							}
							return true;*/
						}

				i8		compare (const sVertex &b) const
						{
							if (data.asU64.idx[0] == b.data.asU64.idx[0])
							{
								if (data.asU64.idx[1] == b.data.asU64.idx[1])
									return 0;
								if (data.asU64.idx[1] > b.data.asU64.idx[1])
									return 1;
								return -1;
							}

							if (data.asU64.idx[0] > b.data.asU64.idx[0])
								return 1;

							return -1;
						}

				u16 	get (u32 position) const 					
						{
							assert(position < NUM_MAX_INDEX_PER_TUPLA); 
							return data.asU16.idx[position];
						}

			private:
				struct sAsU16
				{
					u16		idx[NUM_MAX_INDEX_PER_TUPLA];
				};

				struct sAsU64
				{
					u64		idx[NUM_MAX_INDEX_PER_TUPLA / 4];
				};

				union eData
				{
					sAsU16	asU16;
					sAsU64	asU64;
				};

			private:
				u8 		numIndices;
				//u16		idx[NUM_MAX_INDEX_PER_TUPLA];
				eData	data;
			};


			class DataArrayInterface
			{
			public:
									DataArrayInterface()						{ setInvalid(); }
				virtual 			~DataArrayInterface()						{ }
				
				void 				setInvalid()								{ offsetInTupla = offsetInVtx =0xFF; }

				bool 				isValid() const 							{ return (offsetInTupla!=0xFF); }
				u8 					getOffsetInVtx() const						{ return offsetInVtx; }
				u8 					getOffsetInTupla() const 					{ return offsetInTupla; }
				eVtxLayoutSemantic	getSemantic() const 						{ return semantic; }
				u8					getSemanticIndex() const 					{ return semanticIndex; }
				eVtxLayoutFormat	getFormat() const 							{ return fmt; }

			protected:
				u8					offsetInTupla;
				u8					offsetInVtx;
				eVtxLayoutSemantic 	semantic;
				u8 					semanticIndex;
				eVtxLayoutFormat	fmt;
			};

			template <class T>
			class DataArray : public DataArrayInterface
			{
			public:
						DataArray()	: DataArrayInterface()					 	{ }
				
				void 	setup (	eVtxLayoutSemantic semanticIN, u8 semanticIndexIN, eVtxLayoutFormat fmtIN,
								const T *listIN, u8 offsetInTuplaIN, u8 offsetInVtxIN)		
						{
							semantic = semanticIN; 
							semanticIndex = semanticIndexIN;
							fmt = fmtIN;
							list = listIN; 
							offsetInTupla = offsetInTuplaIN; 
							offsetInVtx = offsetInVtxIN; 
						}

			public:
				const T *list;
			};


			class ImportData
			{
			public:
						ImportData()
						{
							channels.setup (gos::getScrapAllocator(), 16);
						}

						~ImportData()
						{
							reset();
							channels.unsetup();
						}

				void 	reset()
						{
							const u32 n = channels.getNElem();
							for (u32 i=0; i<n; i++)
							{
								GOSDELETE(gos::getScrapAllocator(), channels[i]);
							}
							channels.reset();
						}

						template<class T>
				void	addChannel (eVtxLayoutSemantic semanticIN, u8 semanticIndexIN, eVtxLayoutFormat fmtIN, const T *listIN, u8 offsetInTuplaIN, u8 offsetInVtxIN)
						{
							DataArray<T> *data = GOSNEW(gos::getScrapAllocator(), DataArray<T>)();
							data->setup (semanticIN, semanticIndexIN, fmtIN, listIN, offsetInTuplaIN, offsetInVtxIN);
							channels.append (data);	
						}

			public:
				FastArray<DataArrayInterface*>	channels;
			};


		private:
			void 	priv_reset (eMode modeIN, const VtxLayout &desiredLayout);
			bool	priv_extractTupla (u32 iStart, u32 numIdxPerTupla, sTupla *out) const;
			void 	priv_extractVertex (const sTupla &tupla, sVertex *out) const;
			u32 	priv_findOrCreateVtx (const sVertex &vIN);
			bool	priv_buildFinalShape (gos::Allocator *shapeAllocator, Shape *out_shape);
			bool	priv_endWithTuplaList (gos::Allocator *shapeAllocator, Shape *out_shape);
			bool	priv_endWithRealIdxBuffer (gos::Allocator *shapeAllocator, Shape *out_shape);

					template<class T>
			void 	priv_finalizeShapeVtxBuffer (VtxArrayWriter &writer, const DataArrayInterface *dataArrayIN)
					{
						if (!dataArrayIN->isValid())
							return;

						const DataArray<T> *dataArray = reinterpret_cast<const DataArray<T>*>(dataArrayIN);
						VtxArrayWriter::Elem<T>	elem;
						writer.get (dataArray->getSemantic(), dataArray->getSemanticIndex(), dataArray->getFormat(), &elem);

						const u32 offset = dataArray->getOffsetInVtx();
						const T	*source = dataArray->list;
						for (u32 i=0; i<finalVtxList.getNElem(); i++)
						{
							//const u32 idx = finalVtxList(i).idx[offset];
							const u32 idx = finalVtxList(i).get(offset);
							elem() = source[idx];
							elem.next();				
						}
					}

		private:
			eMode					mode;
			VtxLayout 				shapeVtxLayout;
			FastArray<sVertex>		finalVtxList;
			HashMap<sVertex, u32>	sortedFinalVtxList;
			FastArray<u16>			finalIdxList;
			const u16 				*tupleList;
			u32 					totNumOfIdxInTupleList;
			u8 						numIdxPerOgniTupla;
			u8						errorCode;
			u8						nextValidOffsetInVtx;
			ImportData				importData;
		};
		
	} //namespace shape
 } //namespace gos

#endif //_gosShapeImport_arrays_h_