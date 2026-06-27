#ifndef _gosAsset2Builder_glb_importer_h_
#define _gosAsset2Builder_glb_importer_h_
#include "../../gosShape/gosShape.h"
#include "../../gosShape/skeleton/gosSkeleton.h"
#include "../../gos/gosFastArray.h"
#include "../../gos/gosIniFile.h"
#include "../../gosGeom/gosGeomAABB3.h"
#include "../assetFile/gosAssetFile_materialPBR.h"

namespace gos
{ 
    namespace asset2
    {
		/**
		 * @brief import dal formato glTF binario (file con estensione .glb) v2.0
		 * 
		 * https://www.khronos.org/glTF
		 * glTF format specffication (v2.0): https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
		 * glTF sampler viewer online: https://github.khronos.org/glTF-Sample-Viewer-Release/
		 * 
		 */
		class Importer_glb
		{
		public:

			/***********************
			* @brief	Result
			*			Contiene le shape + shapeName importate dal modello
			*			Contiene lo skeleton importato dal modello
			*/
			struct Result
			{
			public:
				struct sMesh
				{
					u16	shape_index;
					u16 bone_index;
					u16 material_index;
					u16 shape_instance_index;	//==0 => <shape_index> e' usata da 1 sola mesh.
												//==N => <shape_index> e' usata da (N+1) mesh.
				};


			public:
						Result()		{ reset(); }
						~Result()		{ free(); }
				void 	reset()			{ allocator=NULL; numShapes=0; shapeList=NULL; skeleton.reset(); shapeNameList=NULL; bSkeletonIsResolved=false; num_mesh=0; mesh_list=NULL; num_material=0; materialNameList=NULL; material_list=NULL; }
				void 	free()			{ 
					if (NULL == allocator) return;
					if (shapeList)
					{
						for (u32 i = 0; i < numShapes; i++)
						{
							shape::shapeFree(allocator, &shapeList[i]);
							GOSFREE(allocator, shapeNameList[i]);
						}
						for (u32 i = 0; i < num_material; i++)
						{
							GOSFREE(allocator, materialNameList[i]);
						}

						GOSFREE(allocator, shapeList);
						GOSFREE(allocator, shapeNameList);
						GOSFREE(allocator, mesh_list);
						GOSFREE(allocator, materialNameList);
						GOSFREE(allocator, material_list);
						numShapes = num_mesh = num_material = 0;
					}
					gos::skeleton::free(skeleton);
					reset();
				}

				//============== utils ============
				void	calc_AABB (geom::AABB3 *out) const;
				void 	scale (const vec3f &s);
				void 	translate (const vec3f &tr);
				void 	skeleton_resolve();

			public:
				VtxLayout				vtxLayot;
				u32 					numShapes;
				gos::Shape 				*shapeList;
				char					**shapeNameList;
				gos::Skeleton			skeleton;

				u32						num_mesh;
				sMesh					*mesh_list;

				u32						num_material;
				MaterialPBR				*material_list;
				char					**materialNameList;
				
			private:
				bool					priv_is_skeleton_resolved() const { return bSkeletonIsResolved; }
				const gos::Bone*		priv_skeleton_clone_and_resolve (gos::Skeleton *out) const;

			private:
				gos::Allocator 			*allocator;
				bool					bSkeletonIsResolved;

			friend Importer_glb;
			};

		public:
					Importer_glb();
					~Importer_glb();

			bool	importFromFile (const char *filename, const VtxLayout &desiredLayout, gos::Allocator *results_allocator, Result *out_results);
			bool	importFromMemory (const u8 *buffer, u32 sizeof_buffer, const VtxLayout &desiredLayout, gos::Allocator *results_allocator, Result *out_results);

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

			struct sNode
			{
			public:
				static constexpr u32 NUM_MAX_CHILDREN = 64;

			public:
				void 		reset()						{ localTRS.identity(); localRot.identity(); meshIndex = u32MAX; numChildren=0; name[0]=0x00; }
				void 		addChild (u32 index)		{ assert (numChildren < NUM_MAX_CHILDREN); childrenList[numChildren++] = index; }

			public:
				u32			meshIndex;
				mat4x4f 	localTRS;
				Quat 		localRot;
				u32			numChildren;
				u32			childrenList[NUM_MAX_CHILDREN];
				char 		name[64];
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
		
			struct sMeshInfo
			{
				u32 shape_idx;
				u32 material_idx;
			};			

			struct sMesh
			{
			public:
				//per glTF, una mesh e' una collezione di primitive.
				//Per me, la cosa si traduce in una collezione di shape

				void 	begin (gos::FastArray<sMeshInfo> &globalBufferIN)	{ numShapes = 0; offsettInGlobalBuffer = globalBufferIN.getNElem(); globalBuffer = &globalBufferIN; }
				void 	addShape (u32 shape_idx, u32 material_index)		{ numShapes++; sMeshInfo info {shape_idx, material_index};	globalBuffer->append(info); }

				u32 	getNumShapes() const 								{ return numShapes; }
				u32 	getShapeIndex (u32 shapeNum) const					{ return globalBuffer->queryElem (offsettInGlobalBuffer+shapeNum).shape_idx; }
				u32 	getMaterialIndex (u32 shapeNum) const				{ return globalBuffer->queryElem (offsettInGlobalBuffer+shapeNum).material_idx; }

			private:
				gos::FastArray<sMeshInfo> *globalBuffer;
				u32 					numShapes;
				u32 					offsettInGlobalBuffer;
			};
		


		private:
			void 	priv_free();
			bool 	priv_parseBufferView (const gos::IniFileSection *sec);
			bool 	priv_parseAccessor (const gos::IniFileSection *sec);
			bool 	priv_parseMesh (const gos::IniFileSection *sec, const VtxLayout &shape_desired_layout, gos::Allocator *shape_allocator);
			void 	priv_parseMeshAttributes (const gos::IniFileSection *sec, AvailVtxChannel *out) const;
			bool 	priv_parseNodes (const gos::IniFileSection *sec);
			bool 	priv_parseScene (const gos::IniFileSection *sec, Bone *bone);
			bool 	priv_parseMaterial (const gos::IniFileSection *sec);
			void 	priv_resolveNodesHierarcy (Bone *me);

			void 	priv_resolveSkeleton (Bone *rootBone);
			void 	priv_resolveSkeletonChildren (Bone *bone, const Bone *father);
			void	priv_build_mesh_list (Bone *me, const gos::Skeleton *sk, gos::FastArray<Result::sMesh> *out_list);
			
			void 	priv_printStatistics() const;
			void 	priv_printSkeleton() const;
			void 	priv_printSkeleton_rec(const Bone *bone) const;


			bool	priv_build_gosSkeleton (gos::Allocator *sk_allocator, gos::Skeleton *out) const;
			void 	priv_build_gosSkeleton_rec (gos::skeleton::Builder &builder, const Importer_glb::Bone *myBone, u32 skBoneIndex) const;

		private:
			gos::Allocator 					*localAllocator;
			const char 						*json;
			const u8 						*bin;
			u64 							timeStarted_msec;
			gos::FastArray<sBufferView>		bufferViewList;
			gos::FastArray<sAccessors>		accessorsList;
			gos::FastArray<sNode>			nodesList;
			gos::FastArray<sMeshInfo>		shapesInMesh;	//usato come array di appoggio per memorizzare l'elenco delle shape che stanno in ogni singola mesh
			gos::FastArray<sMesh>			meshesList;
			gos::FastArray<Shape> 			shapeList;
			gos::Array<gos::UTF8String>		shapeNameList;

			gos::Array<gos::UTF8String>		materialNameList;
			gos::FastArray<MaterialPBR> 	materialList;
			Bone							rootBone;
			bool							bSkeletonIsResolved;
		};
		
	} //namespace model
 } //namespace gos

#endif //_gosAsset2Builder_glb_importer_h_