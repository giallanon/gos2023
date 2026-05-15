#ifndef _gosAssetFile_model3D_h_
#define _gosAssetFile_model3D_h_
#include "../gosAsset2.h"

namespace gos
{
    namespace asset2
    {
        /********************************
         * @brief AssetFile_model3D
         *
         * Questa classe definisce il formato dell'asset su file
         * E' tipicamente utilizzata dal relativo builder per creare il file asset
         */
        class AssetFile_model3D
        {
        public:
                    AssetFile_model3D();
                    ~AssetFile_model3D()                                    { priv_free(); }

            void    begin (gos::Allocator *localAllocator);

            void    skeleton_set (UID uid_of_concrete_skeleton);

                    //ritorna l'index della shape addata  (eventuali shape duplicate vengono gestite evitando la duplicazione)
            u32     shape_add (UID uid_of_concrete_shape);

            bool    mesh_add (u32 shape_index, u32 bone_index, u32 material_index);

            void    end();


            bool    save (const char *filenameDST);

        private:
            struct sMeshInfo
            {
                u32 shape_index;
                u32 bone_index;
                u32 material_index;
            };

        private:
            void    priv_free();

        private:
            gos::Allocator          *localAllocator;
            UID                     uid_of_concrete_skeleton;
            FastArray<UID>          listof_shape;          
            FastArray<sMeshInfo>    listof_mesh;
            
        };
    } //namespace asset2
} //namespace gos


#endif //_gosAssetFile_model3D_h_


