#ifndef _gosAssetFile_materialPBR_h_
#define _gosAssetFile_materialPBR_h_
#include "gos.h"
#include "../gosAsset2.h"

namespace gos
{
    namespace asset2
    {
		struct MaterialPBR
		{
			f32	diffuse_col_RGBA_HDR[4];
			f32	metallic_factor;

			void reset() {
				diffuse_col_RGBA_HDR[0] = diffuse_col_RGBA_HDR[1] = diffuse_col_RGBA_HDR[2] = diffuse_col_RGBA_HDR[3] = 1.0f;
				metallic_factor = 0;
			}				
		};

        /********************************
         * @brief AssetFile_materialPBR
         *
         * Questa classe definisce il formato dell'asset su file
         * E' tipicamente utilizzata dal relativo builder per creare il file asset
         */
        class AssetFile_materialPBR
        {
		public:
					//serialize
					//se [buffer] == NULL ritorna il num di byte necessari alla serializzazione
					//se [buffer] != NULL ritorna 0 in caso di errore oppure il num di byte memcpyati in [buffer]
			static u32 	serialize (const MaterialPBR &a, u8 *buffer, u32 sizeof_buffer);

					//ritorna 0 in caso di errore
					//altrimenti ritorna il num di byte consumati per la deserializzazione
			static u32 	deserialize (const u8 *buffer, u32 sizeof_buffer, MaterialPBR *out);	


        public:
                    AssetFile_materialPBR()										{ }
                    ~AssetFile_materialPBR()                                    { priv_free(); }

            void    begin ();

			void 	set_from_materialPBR (const MaterialPBR &m);
			void 	set_diffuse_color_HDR_RGBA (f32 r, f32 g, f32 b, f32 a);
			void 	set_metallic_factor_01 (f32 f);

            void    end();


            bool    save (const char *filenameDST);


        private:
            void    priv_free();

        private:
			MaterialPBR	mat;
            
        };
    } //namespace asset2
} //namespace gos

#endif //_gosAssetFile_materialPBR_h_