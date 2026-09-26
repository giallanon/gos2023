#ifndef _gosAsset2_h_
#define _gosAsset2_h_
#include "gosAsset2EnumAndDefine.h"

#define GOS_ASSET2__DEFAULT_DB_NAME         "assets2.sqlite3"
#define GOS_ASSET2__TABLE_RES               "res"
#define GOS_ASSET2__TABLE_DEPENDS           "dependencies"
#define GOS_ASSET2__TABLE_ALIAS             "aliases"
#define GOS_ASSET2__TABLE_SIGNATURE         "signature"
#define GOS_ASSET2__TABLE_SIGNATURE_DEPENDS_RUNTIME   "sig_dependsRT"
#define GOS_ASSET2__TABLE_ASSET_LIST     "asset_list"



namespace gos
{
    namespace asset2
    {
        //================ utils
        const char* enumToString (eResType s);
        const char* enumToString (eAssetType s);
        const char* enumToString (eBuildResult s);

        //================ context
        bool        dbcontext_open_ex (const char *baseFolder, const char *dbName, bool bCreateANewDBIfNotExists, DBContext *out);
        inline bool dbcontext_open (const char *baseFolder, bool bCreateANewDBIfNotExists, DBContext *out)                             { return dbcontext_open_ex (baseFolder, GOS_ASSET2__DEFAULT_DB_NAME, bCreateANewDBIfNotExists, out); }
        void        dbcontext_close (DBContext &ctx);
		bool        dbcontext_query  (DBContext &ctx, const char *query, db::RST &in_out_rst);

        //================ resources
        bool        res_createUID (eResType resType, const char *absFilenameIN, UID *out);
        bool        res_insert (DBContext &ctx, eResType resType, const char *absFilenameIN, u64 lastTimeMod, UID *out_CAN_BE_NULL_uid = NULL);
        bool        res_update (DBContext &ctx, UID uid, u64 lastTimeMod);
        bool        res_exists (DBContext &ctx, eResType resType, const char *absFilenameIN, UID *out_CAN_BE_NULL_uid = NULL);
        bool        res_get_info (DBContext &ctx, UID uid, char *out_CAN_BE_NULL_abspath, u32 sizeof_outabspath, eResType *out_CAN_BE_NULL_resType, u64 *out_CAN_BE_NULL_lastTimeMod);
		bool 		res_is_still_in_use(DBContext &ctx, UID uid);
        
                    //elimina la risorsa UID dal DB eliminando anche le sue dipendenze
        bool        res_delete (DBContext &ctx, const UID &uid);


        //================ signature
        void        signature_manufacture_fullFilename (const DBContext &ctx, UID signatureUID, char *out, u32 sizeof_out);
        bool        signature_createUID (eAssetType assTypeIN, const void *buffer, u32 sizeof_buffer, UID *out);
		bool        signature_exists (DBContext &ctx, UID signatureUID);
		bool        signature_insert (DBContext &ctx, UID signatureUID);

                    //elimina l'asset signatureUID dal DB e da filesystem, eliminando anche le sue dipendenze
        bool        signature_delete (DBContext &ctx, UID signatureUID);
		bool        signature_is_still_in_use (DBContext &ctx, UID signatureUID);

		bool        signature_getBy_rtname (DBContext &ctx, const char *rtname, UID *out__signatureUID);
        bool        signature_get_runtime_dependecies_list (DBContext &ctx, UID signatureUID, bool bClearListOnStart, FastUIDList *out);
        bool        signature_add_dependencyRT (DBContext &ctx, UID signature_padre, UID signature_figlio);


		//================ asset
        bool        asset_insert (DBContext &ctx, eAssetType assType, const char *rtname, UID uid_of_inifile, u32 declared_on_line, UID signatureUID, UID *out_uid);
        bool        asset_get_info (DBContext &ctx, UID uid, UID *out_CAN_BE_NULL_uid_ini, UID *out_CAN_BE_NULL_signatureUID);
        bool        asset_delete (DBContext &ctx, const UID &uid);
        bool        asset_rtname_exists (DBContext &ctx, const char *rtname, UID *out__assetUID, UID *out_CAN_BE_NULL_uid_of_inifile = NULL, UID *out_CAN_BE_NULL_signatureUID = NULL);
		


        //================ alias
        bool        alias_insert (DBContext &ctx, UID uid_of_inifile, const char *alias, const char *absPath);
        bool        alias_get_info (DBContext &ctx, const char *alias, char *out_CAN_BE_NULL_path, u32 sizeof__out_path, UID *out_CAN_BE_NULL_uid_of_inifile);


        //================ dependencies
        bool        dependency_exists (DBContext &ctx, UID father, UID child);
        bool        dependency_add (DBContext &ctx, UID father, UID child);
        
                    //ritorna in <out> un elenco di risorse/asset da cui <uid> dipende (ricorsivamente)
                    template<typename LAMBDA>
        bool        dependency_get_dependecies_list (DBContext &ctx, UID uid, bool bClearListOnStart, UniqueUIDList *out, LAMBDA&& filterFn)
                    {
                        assert (NULL != out);

                        if (bClearListOnStart)
                            out->reset();

                        if (!ctx.isValid())
                        {
                            logger::err ("dependency_get_dependecies_list (%" PRIu64 ") => invalid ctx\n",  uid._uid);
                            return false;
                        }

                        db::RST rst;
                        char s[256];
                        sprintf_s (s, sizeof(s), "SELECT childUID FROM " GOS_ASSET2__TABLE_DEPENDS " WHERE UID=%" PRIu64 "", uid._uid);
                        if (!db::query (ctx.db, s, &rst))
                        {
                            logger::err ("dependency_get_dependecies_list (%" PRIu64 ") => error querying\n",  uid._uid);
                            return false;
                        }

                        while (rst.fetchRow())
                        {
                            UID childUID;
                            childUID._uid = rst.getValAsU64(0);
                            if (filterFn(childUID))
                                out->insertIfNotExists (childUID);
                        }

                        rst.rewind();
                        while (rst.fetchRow())
                        {
                            UID childUID;
                            childUID._uid = rst.getValAsU64(0);
                            if (filterFn(childUID))
                            {
                                if (!dependency_get_dependecies_list (ctx, childUID, false, out, filterFn))
                                    return false;
                            }
                        }
                        return true;
                    }
        inline bool dependency_get_dependecies_list (DBContext &ctx, UID uid, bool bClearListOnStart, UniqueUIDList *out)      { return dependency_get_dependecies_list(ctx, uid, bClearListOnStart, out, [](const UID childUID) { return true; }); }
        
                    //ritorna in <out> tutti le risorse/asset che dipendono da questa risorsa (ricorsivamente)
        bool        dependency_get_requireBy_list (DBContext &ctx, const UID &uid, bool bClearListOnStart, UniqueUIDList *out);
       


    } //namespace asset2
} //namespace gos

#endif //_gosAsset2_h_