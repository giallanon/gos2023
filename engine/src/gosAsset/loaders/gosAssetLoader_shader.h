#ifndef _gosAssetLoader_shader_h_
#define _gosAssetLoader_shader_h_
#include "gosAssetLoaderInterface.h"
#include "../../gosGPU/gosGPU.h"


namespace gos
{
    namespace asset
    {
        /**
        * @brief    Asset_shader;
        * 
        */
        class Asset_shader
        {
        public:
            GPUShaderHandle handle_shader;
        };


        /**
         * @brief Loader_shader
         *
         */
        class Loader_shader : public LoaderInterface
        {
        public:
                    Loader_shader (eAssetType assType) : LoaderInterface (assType)          { }
            virtual ~Loader_shader()                                                        { }

            bool    load (Loader *assetLoader, const asset::Context &ctx, const asset::UID &uid, void *in_out_asset);
            u32     getSizeOfData() const                                                   { return static_cast<u32>(sizeof(Asset_shader)); } 

            
        }; //class Loader_shader


        /**
         * @brief Loader_vtxShader
         *
         */
        class Loader_vtxShader : public Loader_shader
        {
        public:
                    Loader_vtxShader () : Loader_shader (eAssetType::vtx_shader)                 { }
            virtual ~Loader_vtxShader()                                                          { }

        }; //class Loader_vtxShader


        /**
         * @brief Loader_pxlShader
         *
         */
        class Loader_pxlShader : public Loader_shader
        {
        public:
                    Loader_pxlShader () : Loader_shader (eAssetType::pxl_shader)                 { }
            virtual ~Loader_pxlShader()                                                          { }

        }; //class Loader_pxlShader

    } //namespace asset
} //namespace gos

#endif //_gosAssetLoader_shader_h_