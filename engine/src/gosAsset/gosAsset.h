#ifndef _gosAsset_h_
#define _gosAsset_h_
#include "gosAssetEnumAndDefine.h"

namespace gos
{
    namespace asset
    {
        #define GOS_ASSET__TABLE_ASSET_LIST         "assetList"
        #define GOS_ASSET__TABLE_RES_LIST           "resList"
        #define GOS_ASSET__TABLE_RUNTIME_NAME       "rtnameList"
        #define GOS_ASSET__TABLE_DEPENDS            "depends"
        #define GOS_ASSET__TABLE_DEPENDS_RUNTIME    "dependsRT"


        const char* enumToString (const eResType s);
        const char* enumToString (const eAssetType s);
        const char* enumToString (const asset::eBuildResult s);
        bool        stringToEnum (const char *str, eResType *out);
        bool        stringToEnum (const char *str, eAssetType *out);

        bool        context_open    (const char *baseFolder, Context *out);
        bool        context_open_ex (const char *baseFolder, const char *dbName, Context *out);
        bool        context_cloneDB (Context &ctx, const char *file_extension_to_append);
        void        context_close   (Context &ctx);

        
        
        //================ resuorces
        void        res_enumerate_begin (u32 *out_iter);
        bool        res_enumerate_fetch (u32 &iter, eResType *out);
        bool        res_get_folder_nameByType (const char *baseFolder, eResType resType, char *out, u32 sizeof_out);
        bool        res_get_folder_nameByType (const Context &ctx, eResType resType, char *out, u32 sizeof_out);
        bool        res_createUID (eResType resType, const char *filename, asset::UID *out);
        bool        res_insert (Context &ctx, const asset::UID uid, u64 lastTimeMod, eResType resType, const char *resName);
        bool        res_update (Context &ctx, const asset::UID uid, u64 lastTimeMod);
        bool        res_exists (Context &ctx, eResType resType, const char *resName, asset::UID *out_uid);
        bool        res_get_info (Context &ctx, const asset::UID &uid, char *out_CAN_BE_NULL_name, u32 sizeof_outName, eResType *out_CAN_BE_NULL_resType, u64 *out_CAN_BE_NULL_lastTimeMod);
        
                    //ritorna in <out> tutti gli asset che dipendono da questa risorsa (ricorsivamente)
        bool        res_get_requireBy_list (Context &ctx, const asset::UID &uid, bool bClearListOnStart, asset::HashedUIDList *out, asset::eFilter filter);

                    //elimina la risorsa UID dal DB, elimina tutti gli asset che dipendono da lei.
                    //Opzionalmente ritorna in <out> la lista degli asset che sono stati rimossi.
        bool        res_delete (Context &ctx, const asset::UID &uid, asset::HashedUIDList *out_CAN_BE_NULL_list_of_deleted_asset=NULL, bool bClearListOnStart=true);



        //================ iniFile        
        bool        inifile_insert (Context &ctx, const asset::UID uid, u64 lastTimeMod, eAssetType assType, const char *srcfileName);



        //================ assets
        void        asset_get_binfolder_name (const char *baseFolder, char *out, u32 sizeof_out);
        void        asset_get_srcfolder_name (const char *baseFolder, char *out, u32 sizeof_out);

                    //ritorna il filename completo di path della risorsa di uid == uid
        void        asset_manufacture_fullFilename (const Context &ctx, const asset::UID &uid, char *out, u32 sizeof_out);

        bool        asset_createUID (eAssetType assType, u8 asset_depth, const void *buffer, u32 sizeof_buffer, asset::UID *out);
        bool        asset_insert (Context &ctx, const asset::UID &uid, eAssetType assType, u64 lastTimeBuilt, const char *sourceFileInfo);
        
                    //elimina uid dal DB, elimina tutti gli asset che dipendono da uid, elimina tutti i runtimeName associati a UID
                    //Opzionalmente ritorna in <out> la lista degli asset che sono stati rimossi (compreso se stesso)
        bool        asset_delete (Context &ctx, const asset::UID &uid, asset::HashedUIDList *out_CAN_BE_NULL_list_of_deleted_asset=NULL, bool bClearListOnStart=true);
        
                    //ritorna 0 se <uid> non esiste
        u64         asset_query_lastTimeBuilt (Context &ctx, const asset::UID &uid);

        bool        asset_get_info (Context &ctx, const asset::UID &uid, char *out_CAN_BE_NULL_srcFileInfo, u32 sizeof_srcFileInfo, eAssetType *out_CAN_BE_NULL_assType, u64 *out_CAN_BE_NULL_lastTimeBuilt);

                    //ritorna in <out> tutti gli asset e le risorse da cui questo asset dipende (ricorsivamente)
        bool        asset_get_dependecies_list (Context &ctx, const asset::UID &uid, bool bClearListOnStart, asset::HashedUIDList *out);
        bool        asset_get_runtime_dependecies_list (Context &ctx, const asset::UID &uid, bool bClearListOnStart, asset::FastUIDList *out);

                    //ritorna in <out> tutti gli asset che dipendono da questo asset (ricorsivamente)
        bool        asset_get_requireBy_list (Context &ctx, const asset::UID &uid, bool bClearListOnStart, asset::HashedUIDList *out, asset::eFilter filter);
        
                    //ritorna in <out> tutte le risorse di tipo gosasset_d da cui questo asset dipende
        bool        asset_get_script_list (Context &ctx, const asset::UID &uid, bool bClearListOnStart, asset::HashedUIDList *out);


        //================ runtime Name
        bool        rtname_exists (Context &ctx, const char *runtimeName, asset::UID *out_uid);
        bool        rtname_insert (Context &ctx, const char *runtimeName, const asset::UID &uid);

        //================ dependencies
        bool        depend_add (Context &ctx, const asset::UID &padre, const asset::UID &figlio);
        bool        dependRT_add (Context &ctx, const asset::UID &padre, const asset::UID &figlio, u8 depth_figlio);


        
    } //namespace res
} //namespace gos

#endif //_gosAsset_h_