#ifndef _gosAsset2_h_
#define _gosAsset2_h_
#include "gosAsset2EnumAndDefine.h"

#define GOS_ASSET2__DEFAULT_DB_NAME         "assets2.sqlite3"
#define GOS_ASSET2__TABLE_RES               "res"
#define GOS_ASSET2__TABLE_DEPENDS           "dependencies"
#define GOS_ASSET2__TABLE_ASSET_LIST        "assetList"
#define GOS_ASSET2__TABLE_DEPENDS_RUNTIME   "dependsRT"
#define GOS_ASSET2__TABLE_VIRTUAL_ASSET     "virtasset"

namespace gos
{
    namespace asset2
    {
        //================ utils
        const char* enumToString (eResType s);
        const char* enumToString (eAssetType s);
        const char* enumToString (eBuildResult s);

        //================ context
        bool        dbcontext_open_ex (const char *baseFolder, const char *dbName, DBContext *out);
        inline bool dbcontext_open (const char *baseFolder, DBContext *out)                             { return dbcontext_open_ex (baseFolder, GOS_ASSET2__DEFAULT_DB_NAME, out); }
        void        dbcontext_close (DBContext &ctx);


        //================ resources
        bool        res_createUID (eResType resType, const char *absFilenameIN, UID *out);
        bool        res_insert (DBContext &ctx, eResType resType, const char *absFilenameIN, u64 lastTimeMod, UID *out_CAN_BE_NULL_uid = NULL);
        bool        res_update (DBContext &ctx, UID uid, u64 lastTimeMod);
        bool        res_exists (DBContext &ctx, eResType resType, const char *absFilenameIN, UID *out_CAN_BE_NULL_uid = NULL);
        bool        res_get_info (DBContext &ctx, UID uid, char *out_CAN_BE_NULL_abspath, u32 sizeof_outabspath, eResType *out_CAN_BE_NULL_resType, u64 *out_CAN_BE_NULL_lastTimeMod);
        
                    //elimina la risorsa UID dal DB eliminando anche le sue dipendenze
        bool        res_delete (DBContext &ctx, const UID &uid);


        //================ virtual asset
        bool        virtasset_insert (DBContext &ctx, eAssetType assType, const char *rtname, UID uid_of_inifile, u32 declared_on_line, UID uid_of_concrete_asset, UID *out_uid);
        bool        virtasset_get_info (DBContext &ctx, UID uid, UID *out_CAN_BE_NULL_uid_ini, UID *out_CAN_BE_NULL_uid_concrete_asset);
        bool        virtasset_delete (DBContext &ctx, const UID &uid);
        bool        virtasset_rtname_exists (DBContext &ctx, const char *rtname, UID *out_uid);


        //================ asset
        void        asset_manufacture_fullFilename (const DBContext &ctx, UID uid, char *out, u32 sizeof_out);
        bool        asset_createUID (eAssetType assTypeIN, const void *buffer, u32 sizeof_buffer, UID *out);
        bool        asset_exists (DBContext &ctx, UID uid);
        bool        asset_insert (DBContext &ctx, UID uid);
        bool        asset_is_still_in_use(DBContext &ctx, UID uid);

        bool        asset_get_runtime_dependecies_list (DBContext &ctx, UID uid, bool bClearListOnStart, FastUIDList *out);

                    //elimina l'assety UID dal DB e da filesystem, eliminando anche le sue dipendenze
        bool        asset_delete (DBContext &ctx, const UID &uid);


        //================ dependencies
        bool        dependency_exists (DBContext &ctx, UID father, UID child);
        bool        dependency_add (DBContext &ctx, UID father, UID child);
        bool        dependencyRT_add (DBContext &ctx, UID asset_padre, UID asset_figlio);
        
                    //ritorna in <out> un elenco di risorse/asset da cui <uid> dipende (ricorsivamente)
        bool        dependency_get_dependecies_list (DBContext &ctx, UID uid, bool bClearListOnStart, UniqueUIDList *out);
        
                    //ritorna in <out> tutti le risorse/asset che dipendono da questa risorsa (ricorsivamente)
        bool        dependency_get_requireBy_list (DBContext &ctx, const UID &uid, bool bClearListOnStart, UniqueUIDList *out);
        
        //bool       dependency_add_runtimeName (DBContext &ctx, UID padre, UID figlio, u8 depth_figlio);
        // bool        depend_exists  (Context &ctx, UID padre, const UID figlio);
        // bool        depend_add (Context &ctx, UID padre, UID figlio);
        


    } //namespace asset2
} //namespace gos

#endif //_gosAsset2_h_