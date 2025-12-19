#ifndef _gosAsset2_h_
#define _gosAsset2_h_
#include "gosAsset2EnumAndDefine.h"


#define GOS_ASSET2__TABLE_RES               "res"
#define GOS_ASSET2__TABLE_DEPENDS           "depends"


namespace gos
{
    namespace asset2
    {
        //================ context
        bool        dbcontext_open (const char *baseFolder, DBContext *out);
        bool        dbcontext_open_ex (const char *baseFolder, const char *dbName, DBContext *out);
        void        dbcontext_close (DBContext &ctx);


        //================ resources
        bool        res_createUID (eResType resType, const char *absFilenameIN, UID *out);
        bool        res_insert (DBContext &ctx, eResType resType, const char *absFilenameIN, u64 lastTimeMod, UID *out_CAN_BE_NULL_uid = NULL);
        bool        res_update (DBContext &ctx, UID uid, u64 lastTimeMod);
        bool        res_exists (DBContext &ctx, eResType resType, const char *absFilenameIN, UID *out_CAN_BE_NULL_uid = NULL);
        bool        res_get_info (DBContext &ctx, UID uid, char *out_CAN_BE_NULL_abspath, u32 sizeof_outabspath, eResType *out_CAN_BE_NULL_resType, u64 *out_CAN_BE_NULL_lastTimeMod);

        
    } //namespace asset2
} //namespace gos

#endif //_gosAsset2_h_