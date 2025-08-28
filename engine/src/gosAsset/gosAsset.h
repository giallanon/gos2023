#ifndef _gosAsset_h_
#define _gosAsset_h_
#include "gosAssetEnumAndDefine.h"

namespace gos
{
    namespace asset
    {
        #define GOS_ASSET__DB_NAME              "db.db3"
        #define GOS_ASSET__TABLE_ASSET_LIST     "assetList"
        #define GOS_ASSET__TABLE_RES_LIST       "resList"
        #define GOS_ASSET__TABLE_RUNTIME_NAME   "rtnameList"


        const char* enumToString (const eResType s);
        const char* enumToString (const eAssetType s);
        const char* enumToString (const asset::eBuildResult s);
        bool        stringToEnum (const char *str, eResType *out);
        bool        stringToEnum (const char *str, eAssetType *out);

        bool        context_open    (const char *baseFolder, Context *out);
        bool        context_cloneDB (Context &ctx, const char *file_extension_to_append);
        void        context_close   (Context &ctx);

        
        
        //================ resuorces
        void        res_enumerate_begin (u32 *out_iter);
        bool        res_enumerate_fetch (u32 &iter, eResType *out);
        bool        res_get_folder_name (const char *baseFolder, eResType resType, char *out, u32 sizeof_out);
        bool        res_get_folder_name (const Context &ctx, eResType resType, char *out, u32 sizeof_out);
        bool        res_createUID (eResType resType, const char *filename, asset::UID *out);
        bool        res_insert (Context &ctx, const asset::UID uid, u64 lastTimeMod, eResType resType, const char *resName);

        
        //================ iniFile        
        bool        inifile_insert (Context &ctx, const asset::UID uid, u64 lastTimeMod, eAssetType assType, const char *srcfileName);



        //================ assets
        void        asset_get_binfolder_name (const char *baseFolder, char *out, u32 sizeof_out);
        bool        asset_createUID (eAssetType assType, const void *buffer, u32 sizeof_buffer, asset::UID *out);
        bool        asset_insert (Context &ctx, const asset::UID &uid, eAssetType assType, u64 lastTimeBuilt);
        
                    //ritorna 0 se <uid> non esiste
        u64         asset_query_lastTimeBuilt (Context &ctx, const asset::UID &uid);


        //================ runtime Name
                    //se rtname esistva gia' in tabella, ritorna in <out_oldUID> l'uid al quale era precedentemente associato
        bool        rtname_insert_or_update (Context &ctx, const char *runtimeName, const asset::UID &uid, asset::UID *out_oldUID);



        bool        insert_or_update_assets (Context &ctx, const asset::UID &uid, eAssetType assType, u64 lastTimeBuilt);
        bool        add_dependency (Context &ctx, const asset::UID &uidPadre, const asset::UID &uidFiglio);
        bool        add_dependency (Context &ctx, const asset::UID &uidPadre, eResType resType, const char *resname);

        
    } //namespace res
} //namespace gos

#endif //_gosAsset_h_