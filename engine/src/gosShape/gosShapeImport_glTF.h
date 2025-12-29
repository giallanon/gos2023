#ifndef _gosShapeImport_glTF_h_
#define _gosShapeImport_glTF_h_
#include "gosShape.h"
#include "../gos/gosFastArray.h"
#include "../gos/gosIniFile.h"


namespace gos
{ 
	namespace shape
	{
		/**
		 * @brief import dal formato glTF binario (file con estensione .glb) v2.0
		 * 
		 * https://www.khronos.org/glTF
		 * glTF format specffication (v2.0): https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
		 * glTF sampler viewer online: https://github.khronos.org/glTF-Sample-Viewer-Release/
		 * 
		 */
		class glTFImporter
		{
		public:
					glTFImporter();
					~glTFImporter();

			bool	importFromFile (const char *filename, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, gos::ShapeList &out_shapeList);
			bool	importFromMemory (const u8 *buffer, u32 sizeof_buffer, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, gos::ShapeList &out_shapeList);

		private:
			struct sHeader
			{
				u32	magic;
				u32 version;
				u32 length;
			};

			struct sBufferView
			{
				const u8 	*p;
				u32			len;
			};

			struct sAccessors
			{
			public:
				enum class eFmt : u8
				{
					_i8, _u8, _i16, _u16, _i32, _u32, _f32, unknown
				};

				enum class eType : u8
				{
					scalar, vec2, vec3, vec4, matrix2, matrix3, matrix4, unknown
				};

			public:
				static eFmt		parseComponentType (u32 num);
				static eType	parseType (const char *name);

			public:
				eDataFormat		toVtxLayoutFmt() const;

			public:
				const u8	*pData;
				eFmt		fmt;
				u32 		count;
				eType		type;
			};

			class AvailVtxChannel
			{
			public:
						AvailVtxChannel()																				{ reset(); }

				void	reset()																							{ numElem = 0; }
				void 	addAccessorIndex (u32 accessorIndex, eVtxLayoutSemantic semantic, u32 index, eDataFormat fmt);
				bool 	getAccessorIndex (eVtxLayoutSemantic semantic, u32 index, eDataFormat fmt, u32 *out_accessorIndex) const;

			private:
				static constexpr u32 MAX_NUM_ELEM = 32;
			private:
				u32 	numElem;
				u32 	elem[MAX_NUM_ELEM];
			};

			struct sShapeOut
			{
				gos::Allocator 		*shapeAllocator;
				VtxLayout			vtxLayot;
				FastArray<Shape> 	*shapeList;
			};

			struct sNode
			{
			public:
				static constexpr u32 NUM_MAX_CHILDREN = 64;

			public:
				void 		reset()						{ localTRS.identity(); localRot.identity(); meshIndex = u32MAX; numChildren=0; }
				void 		addChild (u32 index)		{ assert (numChildren < NUM_MAX_CHILDREN); childrenList[numChildren++] = index; }

			public:
				u32			meshIndex;
				mat4x4f 	localTRS;
				Quat 		localRot;
				u32			numChildren;
				u32			childrenList[NUM_MAX_CHILDREN];
			};

			struct Bone
			{
			public:
				void	reset()				{ firstChild = nextSibling = NULL; globalTRS.identity(); globalRot.identity(); nodeIndex=u32MAX; }

				void 	addAsChild (Bone *newChildren)
						{
							if (NULL == firstChild)
								firstChild = newChildren;
							else
							{
								Bone *bone = firstChild;
								while (NULL != bone->nextSibling)
									bone = bone->nextSibling;
								bone->nextSibling = newChildren;
							}
						}

				void 	deleteAllChildren (gos::Allocator *allocator)
				{
					Bone *b = firstChild;
					while (b)
					{
						Bone *deleteMe = b;
						b = b->nextSibling;
						deleteMe->deleteAllChildren (allocator);
						GOSDELETE(allocator, deleteMe);
					}
					firstChild = NULL;
				}

			public:
				mat4x4f globalTRS;
				Quat	globalRot;

				Bone 	*firstChild;
				Bone 	*nextSibling;
				u32 	nodeIndex;
			};

		
			struct sMesh
			{
			public:
				//per glTF, una mesh e' una collezione di primitive.
				//Per me, la cosa si traduce in una collezione di shape

				void 	begin (gos::FastArray<u16> &globalBufferIN)		{ numShapes = 0; offsettInGlobalBuffer = globalBufferIN.getNElem(); globalBuffer = &globalBufferIN; }
				void 	addShapeIndex (u16 idx)							{ numShapes++; globalBuffer->append(idx); }
				u32 	getNumShapes() const 							{ return numShapes; }
				u16 	getShapeIndex (u32 shapeNum) const				{ return globalBuffer->queryElem (offsettInGlobalBuffer+shapeNum); }

			private:
				gos::FastArray<u16> *globalBuffer;
				u32 				numShapes;
				u32 				offsettInGlobalBuffer;
			};
		
		private:
			void 	priv_free();
			bool 	priv_parseBufferView (const gos::IniFileSection *sec);
			bool 	priv_parseAccessor (const gos::IniFileSection *sec);
			bool 	priv_parseMesh (const gos::IniFileSection *sec);
			void 	priv_parseMeshAttributes (const gos::IniFileSection *sec, AvailVtxChannel *out) const;
			bool 	priv_parseNodes (const gos::IniFileSection *sec);
			bool 	priv_parseScene (const gos::IniFileSection *sec, Bone *bone);
			void 	priv_resolveNodesHierarcy (Bone *me);

			void 	priv_resolveSkeleton (Bone *rootBone);
			void 	priv_resolveSkeletonChildren (Bone *bone, const Bone *father);
			void 	priv_applySkeleton (Bone *rootBone);
			void 	priv_printStatistics() const;

		private:
			gos::Allocator 					*localAllocator;
			const char 						*json;
			const u8 						*bin;
			u64 							timeStarted_msec;
			sShapeOut						shapeOut;
			gos::FastArray<sBufferView>		bufferViewList;
			gos::FastArray<sAccessors>		accessorsList;
			gos::FastArray<sNode>			nodesList;
			gos::FastArray<u16>				shapesInMesh;	//usato come array di appoggio per memorizzare l'elenco delle shape che stanno in ogni singola mesh
			gos::FastArray<sMesh>			meshesList;
			Bone							rootBone;
		};
		
	} //namespace shape
 } //namespace gos

#endif //_gosShapeImport_glTF_h_