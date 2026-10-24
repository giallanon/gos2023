#ifndef _gosAssetFile_materialPBR_h_
#define _gosAssetFile_materialPBR_h_
#include "gos.h"
#include "../gosAsset2.h"

namespace gos
{
    namespace asset2
    {
        /********************************
         * @brief MaterialPBR
         *
         * Questa classe definisce il formato dell'asset su file
         * E' tipicamente utilizzata dal relativo builder per creare il file asset
         */
		struct MaterialPBR
		{
		public:
					//serialize
					//se [buffer] == NULL ritorna il num di byte necessari alla serializzazione
					//se [buffer] != NULL ritorna 0 in caso di errore oppure il num di byte memcpyati in [buffer]
			static u32 	serialize (const MaterialPBR &a, u8 *buffer, u32 sizeof_buffer);

					//ritorna 0 in caso di errore
					//altrimenti ritorna il num di byte consumati per la deserializzazione
			static u32 	deserialize (const void *buffer, u32 sizeof_buffer, MaterialPBR *out);
			
		public:
			void 	reset();
			bool    save (const char *filenameDST) const;

		public:
			f32	diffuse_col_RGBA_HDR[4];
			f32	metallic_factor;
			UID	diffuse_texture_uid;
			UID	normal_texture_uid;

		};


    } //namespace asset2
} //namespace gos

#endif //_gosAssetFile_materialPBR_h_