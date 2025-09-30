#include "gos.h"
#include "gosAsset.h"
#include "gosUtils.h"
#include "gosRST.h"

using namespace gos;

#define GOS_ASSET__DB_VERSION           4
#define GOS_ASSET__PATH_TO_ASSETS_BIN   "assets/bin"
#define GOS_ASSET__PATH_TO_ASSETS_SRC   "assets/src"


//***************************************************
const char* asset::enumToString (const asset::eResType s)
{
    switch (s)
    {
    default:                                return "!eResType::invalid value";
    case asset::eResType::gosasset_d:       return "gosasset_d";
    case asset::eResType::shader_txt:       return "shader_txt";
    case asset::eResType::image:            return "image";
    }
}

//********************************************************** 
bool asset::stringToEnum (const char *str, asset::eResType *out)
{
	assert (NULL != out);
	if (NULL == str)
		return false;
    if (0 == str[0])
        return false;
	
#define HELPER(value)			if (0 == strcasecmp(str, #value)) { *out=asset::eResType::value ; return true; }

    HELPER(gosasset_d)
    HELPER(shader_txt)
	HELPER(image)
#undef HELPER	
	return false;
}

//***************************************************
const char* asset::enumToString (const eAssetType s)
{
    switch (s)
    {
        default:                            return "!eAssetType::invalid value";
        case eAssetType::__DO__NOT__USE:    return "!__DO__NOT__USE";
        case eAssetType::vtx_shader:        return "vtx_shader";
        case eAssetType::pxl_shader:        return "pxl_shader";
        case eAssetType::tex2D:         return "tex2D";
        case eAssetType::pipe:              return "pipe";
        case eAssetType::shape:             return "shape";
        case eAssetType::DEBUG_ASSET:       return "DEBUG_ASSET";
    }
}

//********************************************************** 
bool asset::stringToEnum (const char *str, eAssetType *out)
{
	assert (NULL != out);
	if (NULL == str)
		return false;
    if (0 == str[0])
        return false;
	
#define HELPER(value)			if (0 == strcasecmp(str, #value)) { *out=eAssetType::value ; return true; }

    //HELPER(__DO__NOT__USE)
    HELPER(vtx_shader)
	HELPER(pxl_shader)
    HELPER(tex2D)
	HELPER(pipe)
    HELPER(shape)
    HELPER(DEBUG_ASSET);

#undef HELPER	
	return false;
}

//***************************************************
const char* asset::enumToString (const asset::eBuildResult s)
{
    switch (s)
    {
    default:                                        return "!eBuildResult::invalid value";
    case asset::eBuildResult::just_built:           return "just_built";
    case asset::eBuildResult::was_already_built:    return "was_already_built";
    case asset::eBuildResult::error:                return "error";
    }
}

//*******************************************************
static bool asset_create_folder_structure (const char *baseFolder)
{
    char s[1024];

    sprintf_s (s, sizeof(s), "%s/" GOS_ASSET__PATH_TO_ASSETS_SRC "", baseFolder);
    if (!fs::folderCreate (s))
        return false;

    sprintf_s (s, sizeof(s), "%s/" GOS_ASSET__PATH_TO_ASSETS_BIN "", baseFolder);
    if (!fs::folderCreate (s))
        return false;


    sprintf_s (s, sizeof(s), "%s/res", baseFolder);
    if (!fs::folderCreate (s))
        return false;

    
    u32 iter;
    asset::eResType resType;
    asset::res_enumerate_begin (&iter);
    while (asset::res_enumerate_fetch(iter, &resType))
    {
        if (asset::res_get_folder_nameByType (baseFolder, resType, s, sizeof(s)))
        {
            if (!fs::folderCreate (s))
                return false;
        }
    }


    return true;
}

//************************************
static bool asset_create_emptyDB (const char *dbFile, DBHandle &db)
{
    char s[1024];

    while (1)
    {
        if (!db::open (dbFile, &db))
            break;

        //table: version
        sprintf_s (s, sizeof(s), "CREATE TABLE version (\
ID INTEGER NOT NULL DEFAULT 1 PRIMARY KEY,\
ver UNSIGNED INT1 NOT NULL DEFAULT 0)\
");
            if (!db::exec (db, s))
                break;

            sprintf_s (s, sizeof(s), "INSERT INTO version (ID,ver) VALUES(1,%d)", GOS_ASSET__DB_VERSION);
            if (!db::exec (db, s))
                break;


            //table: TABLE__RES_LIST
            sprintf_s (s, sizeof(s), "CREATE TABLE " GOS_ASSET__TABLE_RES_LIST " (\
UID UNSIGNED INT8 NOT NULL PRIMARY KEY,\
lastTimeMod UNSIGNED INT8 NOT NULL DEFAULT 0,\
type UNSIGNED INT1 NOT NULL,\
name VARCHAR(128) NOT NULL)\
");
            if (!db::exec (db, s))
                break;
                

            //table: GOS_ASSET__TABLE_RUNTIME_NAME
            sprintf_s (s, sizeof(s), "CREATE TABLE " GOS_ASSET__TABLE_RUNTIME_NAME " (\
name VARCHAR(64) NOT NULL PRIMARY KEY,\
assetUID UNSIGNED INT8 NOT NULL\
)");
            if (!db::exec (db, s))
                break;

            //table: TABLE__ASSET_LIST
            sprintf_s (s, sizeof(s), "CREATE TABLE " GOS_ASSET__TABLE_ASSET_LIST " (\
UID UNSIGNED INT8 NOT NULL PRIMARY KEY,\
lastTimeBuilt UNSIGNED INT8 NOT NULL,\
type UNSIGNED INT1 NOT NULL,\
src VARCHAR(128) NOT NULL)\
");
            if (!db::exec (db, s))
                break;


            //table: GOS_ASSET__TABLE_DEPENDS
            sprintf_s (s, sizeof(s), "CREATE TABLE " GOS_ASSET__TABLE_DEPENDS " (\
UID UNSIGNED INT8 NOT NULL,\
childUID UNSIGNED INT8 NOT NULL,\
PRIMARY KEY('UID','childUID'))");
        if (!db::exec (db, s))
                break;

            //table: GOS_ASSET__TABLE_DEPENDS_RUNTIME 
            sprintf_s (s, sizeof(s), "CREATE TABLE " GOS_ASSET__TABLE_DEPENDS_RUNTIME " (\
UID UNSIGNED INT8 NOT NULL,\
childUID UNSIGNED INT8 NOT NULL,\
childDepth UNSIGNED INT1 NOT NULL,\
PRIMARY KEY('UID','childUID'))");
        if (!db::exec (db, s))
                break;



        //fine di while(1)
        return true;
    }

    db::close(db);
    fs::fileDelete (dbFile);
    return false;
}

//************************************
static bool asset_openDB (DBHandle &db, const char *baseFolder, const char *dbName)
{
    assert (NULL != baseFolder);
    assert (NULL != dbName);

    //apre il DB delle risorse (o lo crea se non esiste gia')
    char fullDBFilePathAndName[1024];
    sprintf_s (fullDBFilePathAndName, sizeof(fullDBFilePathAndName), "%s/%s", baseFolder, dbName);

    if (!fs::fileExists(fullDBFilePathAndName))
    {
        if (!asset_create_emptyDB(fullDBFilePathAndName, db))
            return false;
    }
    else
    {
        if (!db::open (fullDBFilePathAndName, &db))
            return false;

        //verifico che la versione sia corretta
        db::RST rst;
        if (db::query (db, "SELECT ver FROM version WHERE ID=1", &rst))
        {
            if (rst.fetchRow())
            {
                if (GOS_ASSET__DB_VERSION != rst.getValAsU32(0))
                {
                    gos::logger::err ("asset::asset_openDB(): wrong DB version, expected ver=%d\n", GOS_ASSET__DB_VERSION);
                    return false;
                }
            }
        }
    }

    return true;
}




//*******************************************************
bool asset::context_open_ex (const char *baseFolderIN, const char *dbName, Context *out)
{
    assert (NULL != baseFolderIN);
    assert (NULL != dbName);
    assert (NULL != out);

    if (out->isValid())
        return false;

    char s[1024];
    fs::resolvePath (baseFolderIN, s, sizeof(s));
    
    //crea la struttura di cartelle se non esiste gia'
    asset_create_folder_structure (s);

    //apre il db o lo crea se non esiste gia'
    if (!asset_openDB(out->db, s, dbName))
        return false;

    out->dbName = string::utf8::allocStr (gos::getSysHeapAllocator(), dbName);

    out->baseFolder = string::utf8::allocStr (gos::getSysHeapAllocator(), s);

    asset_get_srcfolder_name (out->baseFolder, s, sizeof(s));
    out->folder_assets_src = string::utf8::allocStr (gos::getSysHeapAllocator(), s);

    asset_get_binfolder_name (out->baseFolder, s, sizeof(s));
    out->folder_assets_bin = string::utf8::allocStr (gos::getSysHeapAllocator(), s);

    sprintf_s (s, sizeof(s), "%s/res", out->baseFolder);
    out->folder_res = string::utf8::allocStr (gos::getSysHeapAllocator(), s);
    
    
    return true; 
}

//*******************************************************
bool asset::context_open (const char *baseFolderIN, Context *out)
{
    return context_open_ex (baseFolderIN, "assets.sqlite3", out);
}

//*******************************************************
void asset::context_close (Context &ctx)
{
    if (!ctx.isValid())
        return;

    GOSFREE (gos::getSysHeapAllocator(), ctx.baseFolder);               ctx.baseFolder = NULL;
    GOSFREE (gos::getSysHeapAllocator(), ctx.dbName);                   ctx.dbName = NULL;
    GOSFREE (gos::getSysHeapAllocator(), ctx.folder_assets_src);        ctx.folder_assets_src = NULL;
    GOSFREE (gos::getSysHeapAllocator(), ctx.folder_assets_bin);        ctx.folder_assets_bin = NULL;
    GOSFREE (gos::getSysHeapAllocator(), ctx.folder_res);               ctx.folder_res = NULL;

    db::close (ctx.db);
}

//*******************************************************
bool asset::context_cloneDB  (Context &ctx, const char *file_extension_to_append)
{
    if (!ctx.isValid())
        return false;

    char src[1024];
    sprintf_s (src, sizeof(src), "%s/%s", ctx.baseFolder, ctx.dbName);

    char dst[1024];
    sprintf_s (dst, sizeof(dst), "%s/%s%s", ctx.baseFolder, ctx.dbName, file_extension_to_append);

    db::close (ctx.db);
    const bool ret = fs::fileCopy (src, dst);

    db::open (src, &ctx.db);
    return ret;
}






//********************************************************** 
void asset::res_enumerate_begin (u32 *out_iter) 
{
    assert (NULL != out_iter);
    *out_iter = 2;
}

//********************************************************** 
bool asset::res_enumerate_fetch (u32 &iter, eResType *out)
{
    if (iter >= static_cast<u32>(asset::eResType::__FINISHED))
        return false;

    *out = static_cast<asset::eResType>(iter++);
    return true;
}    

//********************************************************** 
bool asset::res_get_folder_nameByType (const Context &ctx, eResType resType, char *out, u32 sizeof_out)
{
    if (!ctx.isValid())
    {
        DBGBREAK;
        out[0] = 0x00;
        return false;
    }
    return asset::res_get_folder_nameByType (ctx.baseFolder, resType, out, sizeof_out);
}
bool asset::res_get_folder_nameByType (const char *baseFolder, eResType resType, char *out, u32 sizeof_out)
{
    assert (NULL != out);

    const char *resTypeName = asset::enumToString(resType);
    if (resTypeName[0] == '!')
        return false;   //vuol dire che enumToString() non ha riconosciuto il resType

    sprintf_s (out, sizeof_out, "%s/res/%02d-%s", baseFolder, static_cast<u8>(resType), resTypeName);
    return true;
}

//********************************************************** 
bool asset::res_createUID (eResType resTypeIN, const char *filename, asset::UID *out)
{
    assert (NULL != filename);
    assert (NULL != out);

    char s[128];
    sprintf_s (s, sizeof(s), "%02d_%s", static_cast<u8>(resTypeIN), filename);

    //crc del nome
    out->_uid = utils::crc32(s, static_cast<u32>(strlen(s)));

    //uso il terzo byte MSB per metterci l'assType
    u64 resourceType = static_cast<u64>(resTypeIN);
    resourceType <<= 40;
    out->_uid |= resourceType;

    return true;
}

//********************************************************** 
bool asset::res_insert (Context &ctx, const asset::UID uid, u64 lastTimeMod, eResType resType, const char *resName)
{
    assert (uid.isAResource());

    if (!ctx.isValid())
    {
        logger::err ("asset::res_insert(%" PRIu64 ", %02d, '%s') => invalid ctx\n",  uid._uid, resType, resName);
        return false;
    }

    char s[512];
    sprintf_s (s, sizeof(s), "INSERT INTO " GOS_ASSET__TABLE_RES_LIST " (UID,lastTimeMod,type,name) VALUES(%" PRIu64 ",%" PRIu64 ",%d,'%s')", 
                    uid._uid, 
                    lastTimeMod, 
                    static_cast<u8>(resType),
                    resName);
    if (!db::exec (ctx.db, s))
    {
        logger::err ("asset::res_insert(%" PRIu64 ", %02d, '%s') => error inserting into table\n",  uid._uid, resType, resName);
        return false;
    }

    return true;
}

//********************************************************** 
bool asset::res_update (Context &ctx, const asset::UID uid, u64 lastTimeMod)
{
    assert (uid.isAResource());

    if (!ctx.isValid())
    {
        logger::err ("asset::res_update(%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    char s[512];
    sprintf_s (s, sizeof(s), "UPDATE " GOS_ASSET__TABLE_RES_LIST " SET lastTimeMod=%" PRIu64 " WHERE UID=%" PRIu64 ";", lastTimeMod, uid._uid);
    if (!db::exec (ctx.db, s))
    {
        logger::err ("asset::res_update(%" PRIu64 ") => error updating table\n",  uid._uid);
        return false;
    }

    return true;
}

//********************************************************** 
bool asset::res_exists (Context &ctx, eResType resType, const char *resName, asset::UID *out_uid)
{
    assert (NULL != out_uid);

    out_uid->setInvalid();

    if (!ctx.isValid())
    {
        logger::err ("res_exists(\"%s\") => invalid ctx\n", resName);
        return false;
    }

    db::RST rst;
    char s[256];
    
    sprintf_s (s, sizeof(s), "SELECT UID FROM " GOS_ASSET__TABLE_RES_LIST " WHERE type=%d AND name='%s'", static_cast<u8>(resType), resName);
    if (!db::query (ctx.db, s, &rst)) return false;
    if (rst.fetchRow())
    {
        out_uid->_uid = rst.getValAsU64(0);
        return true;
    }

    return false;
}

//********************************************************** 
bool asset::res_get_info (Context &ctx, const asset::UID &uid, char *out_CAN_BE_NULL_name, u32 sizeof_outName, eResType *out_CAN_BE_NULL_resType, u64 *out_CAN_BE_NULL_lastTimeMod)
{
    assert (uid.isAResource());

    if (!ctx.isValid())
    {
        logger::err ("asset::res_get_name(%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    db::RST rst;
    char s[128];
    
    sprintf_s (s, sizeof(s), "SELECT lastTimeMod,type,name FROM " GOS_ASSET__TABLE_RES_LIST " WHERE UID=%" PRIu64 "", uid._uid);
    if (!db::query (ctx.db, s, &rst)) return false;
    if (rst.fetchRow())
    {
        if (NULL != out_CAN_BE_NULL_lastTimeMod)    *out_CAN_BE_NULL_lastTimeMod = rst.getValAsU64(0);
        if (NULL != out_CAN_BE_NULL_resType)        *out_CAN_BE_NULL_resType = static_cast<eResType>(rst.getValAsU8(1));
        if (NULL != out_CAN_BE_NULL_name)           sprintf_s (out_CAN_BE_NULL_name, sizeof_outName, "%s", rst.getVal(2));
        return true;
    }

    return false;
}

//********************************************************** 
bool asset::res_get_requireBy_list (Context &ctx, const asset::UID &uid, bool bClearListOnStart, asset::HashedUIDList *out, asset::eFilter filter)
{
    assert (uid.isAResource());
    assert (NULL != out);

    if (bClearListOnStart)
        out->reset();

    if (!ctx.isValid())
    {
        logger::err ("res_get_requireBy_list (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    db::RST rst;
    char s[256];
    sprintf_s (s, sizeof(s), "SELECT UID FROM " GOS_ASSET__TABLE_DEPENDS " WHERE childUID=%" PRIu64 "", uid._uid);
    if (!db::query (ctx.db, s, &rst))
    {
        logger::err ("res_get_requireBy_list (%" PRIu64 ") => error querying\n",  uid._uid);
        return false;
    }

    while (rst.fetchRow())
    {
        asset::UID childUID;
        childUID._uid = rst.getValAsU64(0);
        
        switch (filter)
        {
        default:
            DBGBREAK;
            break;

        case asset::eFilter::both:
            out->insertIfNotExists (childUID, 0);
            break;

        case asset::eFilter::only_assets:
            if (childUID.isAnAsset())
                out->insertIfNotExists (childUID, 0);
            break;

        case asset::eFilter::only_resources:
            if (childUID.isAResource())
                out->insertIfNotExists (childUID, 0);
            break;
        }

    }

    rst.rewind();
    while (rst.fetchRow())
    {
        asset::UID childUID;
        childUID._uid = rst.getValAsU64(0);

        if (childUID.isAnAsset())
        {
            if (!asset_get_requireBy_list (ctx, childUID, false, out, filter))
                return false;
        }
        else
        {
            if (!res_get_requireBy_list (ctx, childUID, false, out, filter))
                return false;
        }
    }
    return true;
}

//*******************************************************
bool asset::res_delete (Context &ctx, const asset::UID &uid, asset::HashedUIDList *out_CAN_BE_NULL_list_of_deleted_asset, bool bClearListOnStart)
{
    assert (uid.isAResource());

    if (!ctx.isValid())
    {
        logger::err ("res_delete (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }
    
    if (!uid.isAResource())
    {
        logger::err ("res_delete (%" PRIu64 ") => UID is not an RESOURCE uid\n",  uid._uid);
        return false;
    }

    //prima di eliminare la risorsa, recupero una lista di asset che dipendono da me, perche' vanno eliminati anche quelli
    asset::HashedUIDList dependList(gos::getSysHeapAllocator(), 128);
    if (!asset::res_get_requireBy_list (ctx, uid, true, &dependList, asset::eFilter::only_assets))
    {
        logger::err ("res_delete (%" PRIu64 ") => error querying requiderByList\n",  uid._uid);
        return false;
    }

    if (!db::transaction_begin(ctx.db))
    {
        logger::err ("res_delete (%" PRIu64 ") => transaction_begin failed, DB has not been touched\n",  uid._uid);
        return false;
    }

    bool ret = true;
    //elimino tutti gli asset da cui dipendo
    auto depList = dependList._queryList();
    const u32 n = depList->getNElem();
    for (u32 i=0; i<n; i++)
    {
        if (!asset::asset_deleteFromDB (ctx, uid, out_CAN_BE_NULL_list_of_deleted_asset, bClearListOnStart))
        {
            ret = false;
            break;
        }
    } 

    if (ret)
    {
        //elimino me stesso
        char s[128];
        sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET__TABLE_RES_LIST " WHERE UID=%" PRIu64 "", uid._uid);
        ret = db::exec (ctx.db, s);

        if (ret)
        {
            sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET__TABLE_DEPENDS " WHERE UID=%" PRIu64 "", uid._uid);
            ret = db::exec (ctx.db, s);
        }
    }


    if (!ret)
    {
        db::transaction_rollback(ctx.db);
        logger::err ("res_delete (%" PRIu64 ") => error deleting. DB has been rolled-back\n",  uid._uid);
        return false;
    }

    if (!db::transaction_commit (ctx.db))
    {
        logger::err ("res_delete (%" PRIu64 ") => transaction_commit failed... not sure what to do\n",  uid._uid);
        return false;
    }
        
    return true;
}




//*******************************************************
void asset::asset_manufacture_fullFilename (const Context &ctx, const asset::UID &uid, char *out, u32 sizeof_out)
{
    sprintf_s (out, sizeof_out, "%s/%016" PRIX64 ".gosasset", ctx.folder_assets_bin, uid._uid);
}

//*******************************************************
void asset::asset_get_binfolder_name (const char *baseFolder, char *out, u32 sizeof_out)
{
    sprintf_s (out, sizeof_out, "%s/" GOS_ASSET__PATH_TO_ASSETS_BIN "", baseFolder);
}

//*******************************************************
void asset::asset_get_srcfolder_name (const char *baseFolder, char *out, u32 sizeof_out)
{
    sprintf_s (out, sizeof_out, "%s/" GOS_ASSET__PATH_TO_ASSETS_SRC "", baseFolder);
}

//*******************************************************
bool asset::asset_createUID (eAssetType assTypeIN, u8 asset_depth, const void *buffer, u32 sizeof_buffer, asset::UID *out)
{
    assert (out != NULL);

    if (NULL == out || NULL == buffer || sizeof_buffer < 4)
    {
        out->_uid = 0;
        return false;
    }

    const u32 sizeof_blob = 8+ sizeof_buffer;
    u8 *blob = GOSALLOC_SCRAPT(u8*, sizeof_blob);
    blob[0] = static_cast<u8>(assTypeIN);
    blob[1] = 0;
    blob[2] = 0;
    blob[3] = 0;
    blob[4] = 0;
    blob[5] = 0;
    blob[6] = 0;
    blob[7] = 0;
    memcpy (&blob[8], buffer, sizeof_buffer);

    //crc del blob
    out->_uid = utils::crc32(blob, sizeof_blob);

    //uso il penultimo byte MSB per metterci l'assType
    u64 uu = static_cast<u64>(assTypeIN);
    uu <<= 48;
    out->_uid |= uu;

    //ci metto anche l'asset depth
    uu = static_cast<u64>(asset_depth);
    uu <<= 32;
    out->_uid |= uu;

    GOSFREE_SCRAP(blob);
    return true;
}

//*******************************************************
bool asset::asset_insert (Context &ctx, const asset::UID &uid, eAssetType assType, u64 lastTimeBuilt, const char *sourceFileInfo)
{
    assert (uid.isAnAsset());
    assert (NULL != sourceFileInfo);

    if (u64MAX == lastTimeBuilt)
    {
        gos::DateTime dt;
        dt.setNow_UTC();
        lastTimeBuilt = dt.getAsNiceU64();
    }

    if (!ctx.isValid())
    {
        logger::err ("asset_insert (%" PRIu64 ", %d, %" PRIu64 ", '%s') => invalid ctx\n",  uid._uid, assType, lastTimeBuilt, sourceFileInfo);
        return false;
    }

    db::RST rst;
    char s[256];
    sprintf_s (s, sizeof(s), "INSERT INTO " GOS_ASSET__TABLE_ASSET_LIST " (UID,lastTimeBuilt,type,src) VALUES(%" PRIu64 ",%" PRIu64 ",%d,'%s')", uid._uid, lastTimeBuilt, static_cast<u8>(assType), sourceFileInfo);
    if (!db::exec (ctx.db, s))
    {
        logger::err ("asset_insert(%" PRIu64 ", %d, %" PRIu64 ", '%s') => error inserting into table\n", uid._uid, assType, lastTimeBuilt, sourceFileInfo);
        return false;
    }

    return true;
}

//*******************************************************
bool asset__do_asset_delete_ric (asset::Context &ctx, const asset::UID &uid, asset::HashedUIDList &outList)
{
    assert (uid.isAnAsset());

    if (!outList.insertIfNotExists (uid, 0))
        return true;    //vuol dire che sono stato gia' processato ed eliminato

    //prima di eliminare l'asset, recupero una lista di asset che dipendono da me, perche' vanno eliminati anche quelli
    asset::HashedUIDList dependList(gos::getSysHeapAllocator(), 32);
    if (!asset::asset_get_requireBy_list (ctx, uid, true, &dependList, asset::eFilter::only_assets))
        return false;

    //elimino me stesso
    char s[256];
    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET__TABLE_ASSET_LIST " WHERE UID=%" PRIu64 "", uid._uid);
    if (!db::exec (ctx.db, s))
        return false;

    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET__TABLE_DEPENDS " WHERE UID=%" PRIu64 "", uid._uid);
    if (!db::exec (ctx.db, s))
        return false;

    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET__TABLE_DEPENDS_RUNTIME " WHERE UID=%" PRIu64 "", uid._uid);
    if (!db::exec (ctx.db, s))
        return false;

    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET__TABLE_RUNTIME_NAME " WHERE assetUID=%" PRIu64 "", uid._uid);
    if (!db::exec (ctx.db, s))
        return false;

    //elimino tutti gli asset da cui dipendo
    auto depList = dependList._queryList();
    const u32 n = depList->getNElem();
    for (u32 i=0; i<n; i++)
    {
        if (!asset__do_asset_delete_ric (ctx, depList->queryElem(i).key, outList))
            return false;
    } 

    return true;
}

bool asset::asset_deleteFromDB (Context &ctx, const asset::UID &uid, asset::HashedUIDList *out_CAN_BE_NULL_list_of_deleted_asset, bool bClearListOnStart)
{
    assert (uid.isAnAsset());

    if (!ctx.isValid())
    {
        logger::err ("asset_deleteFromDB (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }
    
    if (!uid.isAnAsset())
    {
        logger::err ("asset_deleteFromDB (%" PRIu64 ") => UID is not an ASSET uid\n",  uid._uid);
        return false;
    }

    if (!db::transaction_begin(ctx.db))
    {
        logger::err ("asset_deleteFromDB (%" PRIu64 ") => transaction_begin failed, DB has not been touched\n",  uid._uid);
        return false;
    }

    bool ret;
    if (NULL == out_CAN_BE_NULL_list_of_deleted_asset)
    {
        asset::HashedUIDList outList (gos::getSysHeapAllocator(), 64);
        ret = asset__do_asset_delete_ric (ctx, uid, outList);
    }
    else
    {
        if (bClearListOnStart)
            out_CAN_BE_NULL_list_of_deleted_asset->reset();
        ret = asset__do_asset_delete_ric (ctx, uid, *out_CAN_BE_NULL_list_of_deleted_asset);
    }

    if (!ret)
    {
        db::transaction_rollback(ctx.db);
        logger::err ("asset_deleteFromDB (%" PRIu64 ") => error deleting asset. DB has been rolled-back\n",  uid._uid);
        return false;
    }        

    if (!db::transaction_commit (ctx.db))
    {
        logger::err ("asset_deleteFromDB (%" PRIu64 ") => transaction_commit failed... not sure what to do\n",  uid._uid);
        return false;
    }
        
    return true;
}

//*******************************************************
u64 asset::asset_query_lastTimeBuilt (Context &ctx, const asset::UID &uid)
{
    assert (uid.isAnAsset());

    if (!ctx.isValid())
    {
        logger::err ("asset_query_lastTimeBuilt (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return 0;
    }

    db::RST rst;
    char s[256];
    sprintf_s (s, sizeof(s), "SELECT lastTimeBuilt FROM " GOS_ASSET__TABLE_ASSET_LIST " WHERE UID=%" PRIu64 "", uid._uid);
    if (!db::query (ctx.db, s, &rst))
    {
        logger::err ("asset_query_lastTimeBuilt (%" PRIu64 ") => error querying\n",  uid._uid);
        return 0;
    }

    if (rst.fetchRow())
        return rst.getValAsU64(0);
    return 0;
}

//*******************************************************
bool asset::asset_get_info (Context &ctx, const asset::UID &uid, char *out_CAN_BE_NULL_srcFileInfo, u32 sizeof_srcFileInfo, eAssetType *out_CAN_BE_NULL_assType, u64 *out_CAN_BE_NULL_lastTimeBuilt)
{
    assert (uid.isAnAsset());

    if (!ctx.isValid())
    {
        logger::err ("asset_get_info (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return 0;
    }

    db::RST rst;
    char s[256];
    sprintf_s (s, sizeof(s), "SELECT lastTimeBuilt,type,src FROM " GOS_ASSET__TABLE_ASSET_LIST " WHERE UID=%" PRIu64 "", uid._uid);
    if (!db::query (ctx.db, s, &rst))
    {
        logger::err ("asset_get_info (%" PRIu64 ") => error querying\n",  uid._uid);
        return 0;
    }

    if (!rst.fetchRow())
        return false;

    if (NULL != out_CAN_BE_NULL_lastTimeBuilt)
        *out_CAN_BE_NULL_lastTimeBuilt = rst.getValAsU64(0);

    if (NULL != out_CAN_BE_NULL_assType)
        *out_CAN_BE_NULL_assType = static_cast<eAssetType>(rst.getValAsU8(1));

    if (NULL != out_CAN_BE_NULL_srcFileInfo)
        sprintf_s (out_CAN_BE_NULL_srcFileInfo, sizeof_srcFileInfo, "%s", rst.getVal(2));

    return true;
}

//*******************************************************
bool asset::asset_get_dependecies_list (Context &ctx, const asset::UID &uid, bool bClearListOnStart, asset::HashedUIDList *out)
{
    assert (NULL != out);
    //assert (uid.isAnAsset());

    if (bClearListOnStart)
        out->reset();

    if (!ctx.isValid())
    {
        logger::err ("asset_get_dependecies_list (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    db::RST rst;
    char s[256];
    sprintf_s (s, sizeof(s), "SELECT childUID FROM " GOS_ASSET__TABLE_DEPENDS " WHERE UID=%" PRIu64 "", uid._uid);
    if (!db::query (ctx.db, s, &rst))
    {
        logger::err ("asset_get_dependecies_list (%" PRIu64 ") => error querying\n",  uid._uid);
        return false;
    }

    while (rst.fetchRow())
    {
        asset::UID childUID;
        childUID._uid = rst.getValAsU64(0);
        out->insertIfNotExists (childUID, 0);
    }

    rst.rewind();
    while (rst.fetchRow())
    {
        asset::UID childUID;
        childUID._uid = rst.getValAsU64(0);
        
        if (childUID.isAnAsset())
        {
            if (!asset_get_dependecies_list (ctx, childUID, false, out))
                return false;
        }
    }
    return true;
}

//*******************************************************
bool asset::asset_get_runtime_dependecies_list (Context &ctx, const asset::UID &uid, bool bClearListOnStart, asset::FastUIDList *out)
{
    assert (NULL != out);
    assert (uid.isAnAsset());

    if (bClearListOnStart)
        out->reset();

    if (!ctx.isValid())
    {
        logger::err ("asset_get_runtime_dependecies_list (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    db::RST rst;
    char s[256];
    sprintf_s (s, sizeof(s), "SELECT childUID FROM " GOS_ASSET__TABLE_DEPENDS_RUNTIME " WHERE UID=%" PRIu64 " ORDER BY childDepth ASC", uid._uid);
    if (!db::query (ctx.db, s, &rst))
    {
        logger::err ("asset_get_runtime_dependecies_list (%" PRIu64 ") => error querying\n",  uid._uid);
        return false;
    }

    while (rst.fetchRow())
    {
        asset::UID childUID;
        childUID._uid = rst.getValAsU64(0);
        out->append (childUID);
    }

    return true;
}

//*******************************************************
bool asset::asset_get_script_list (Context &ctx, const asset::UID &uid, bool bClearListOnStart, asset::HashedUIDList *out)
{
    assert (NULL != out);
    assert (uid.isAnAsset());

    if (bClearListOnStart)
        out->reset();

    if (!ctx.isValid())
    {
        logger::err ("asset_get_script_list (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    db::RST rst;
    char s[256];
    sprintf_s (s, sizeof(s), "SELECT childUID FROM " GOS_ASSET__TABLE_DEPENDS " WHERE UID=%" PRIu64 "", uid._uid);    
    if (!db::query (ctx.db, s, &rst))
    {
        logger::err ("asset_get_script_list (%" PRIu64 ") => error querying\n",  uid._uid);
        return false;
    }

    while (rst.fetchRow())
    {
        asset::UID childUID;
        childUID._uid = rst.getValAsU64(0);

        if (childUID.isAResourceOfType (eResType::gosasset_d))
            out->insertIfNotExists (childUID, 0);
    }

    rst.rewind();
    while (rst.fetchRow())
    {
        asset::UID childUID;
        childUID._uid = rst.getValAsU64(0);

        if (childUID.isAnAsset())
        {
            if (!asset_get_script_list (ctx, childUID, false, out))
                return false;
        }
    }
    return true;
}

//*******************************************************
bool asset::asset_get_requireBy_list (Context &ctx, const asset::UID &uid, bool bClearListOnStart, asset::HashedUIDList *out, asset::eFilter filter)
{
    assert (uid.isAnAsset());
    assert (NULL != out);

    if (bClearListOnStart)
        out->reset();

    if (!ctx.isValid())
    {
        logger::err ("asset_get_requireBy_list (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    db::RST rst;
    char s[256];
    sprintf_s (s, sizeof(s), "SELECT UID FROM " GOS_ASSET__TABLE_DEPENDS " WHERE childUID=%" PRIu64 "", uid._uid);
    if (!db::query (ctx.db, s, &rst))
    {
        logger::err ("asset_get_requireBy_list (%" PRIu64 ") => error querying\n",  uid._uid);
        return false;
    }

    while (rst.fetchRow())
    {
        asset::UID childUID;
        childUID._uid = rst.getValAsU64(0);
        
        switch (filter)
        {
        default:
            DBGBREAK;
            break;

        case asset::eFilter::both:
            out->insertIfNotExists (childUID, 0);
            break;

        case asset::eFilter::only_assets:
            if (childUID.isAnAsset())
                out->insertIfNotExists (childUID, 0);
            break;

        case asset::eFilter::only_resources:
            if (childUID.isAResource())
                out->insertIfNotExists (childUID, 0);
            break;
        }

    }

    rst.rewind();
    while (rst.fetchRow())
    {
        asset::UID childUID;
        childUID._uid = rst.getValAsU64(0);

        if (childUID.isAnAsset())
        {
            if (!asset_get_requireBy_list (ctx, childUID, false, out, filter))
                return false;
        }
    }
    return true;
}




//*******************************************************
bool asset::rtname_exists (Context &ctx, const char *runtimeName, asset::UID *out_uid)
{
    assert (NULL != out_uid);
    out_uid->setInvalid();

    if (!ctx.isValid())
    {
        logger::err ("rtname_exists(\"%s\") => invalid ctx\n", runtimeName);
        return false;
    }

    db::RST rst;
    char s[256];
    
    sprintf_s (s, sizeof(s), "SELECT assetUID FROM " GOS_ASSET__TABLE_RUNTIME_NAME " WHERE name='%s'", runtimeName);
    if (!db::query (ctx.db, s, &rst)) return false;
    if (rst.fetchRow())
    {
        out_uid->_uid = rst.getValAsU64(0);
        return true;
    }

    return false;
}

//*******************************************************
bool asset::rtname_insert (Context &ctx, const char *runtimeName, const asset::UID &uid)
{
    assert (uid.isAnAsset());

    if (!ctx.isValid())
    {
        logger::err ("rtname_insert(\"%s\", %" PRIu64 ") => invalid ctx\n", runtimeName, uid._uid);
        return false;
    }

    char s[256];
    //non esisteva, lo aggiungo
    sprintf_s (s, sizeof(s), "INSERT INTO " GOS_ASSET__TABLE_RUNTIME_NAME " (name,assetUID) VALUES('%s', %" PRIu64 ")", runtimeName, uid._uid);
    if (db::exec (ctx.db, s))
        return true;

    logger::err ("rtname_insert(\"%s\", %" PRIu64 ") => error inserting into table\n", runtimeName, uid._uid);
    return false;
}


//*******************************************************
bool asset::depend_add (Context &ctx, const asset::UID &uid_padre, const asset::UID &uid_figlio)
{
    if (!ctx.isValid())
    {
        logger::err ("depend_add(%016" PRIX64 ",%016" PRIX64 ") => invalid ctx\n", uid_padre._uid, uid_figlio._uid);
        return false;
    }

    char s[128];
    sprintf_s (s, sizeof(s), "INSERT INTO " GOS_ASSET__TABLE_DEPENDS " (UID,childUID) VALUES(%" PRIu64 ",%" PRIu64 ")", uid_padre._uid, uid_figlio._uid);
    if (db::exec (ctx.db, s))
        return true;

    logger::err ("depend_add(%016" PRIX64 ",%016" PRIX64 ") => error inserting into table\n", uid_padre._uid, uid_figlio._uid);
    return false;
}

//*******************************************************
bool asset::dependRT_add (Context &ctx, const asset::UID &uid_padre, const asset::UID &uid_figlio, u8 depth_figlio)
{
    if (!ctx.isValid())
    {
        logger::err ("dependRT_add(%016" PRIX64 ",%016" PRIX64 ") => invalid ctx\n", uid_padre._uid, uid_figlio._uid);
        return false;
    }

    char s[128];
    sprintf_s (s, sizeof(s), "INSERT INTO " GOS_ASSET__TABLE_DEPENDS_RUNTIME " (UID,childUID,childDepth) VALUES(%" PRIu64 ",%" PRIu64 ",%d)", uid_padre._uid, uid_figlio._uid, depth_figlio);
    if (db::exec (ctx.db, s))
        return true;

    logger::err ("dependRT_add(%016" PRIX64 ",%016" PRIX64 ") => error inserting into table\n", uid_padre._uid, uid_figlio._uid);
    return false;
}
