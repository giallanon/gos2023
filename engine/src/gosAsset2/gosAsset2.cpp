#include "gos.h"
#include "gosAsset2.h"

using namespace gos;

#define GOS_ASSET2__DB_VERSION           1


//***********************************
const char* asset2::enumToString (eResType s)
{
	switch (s)
	{
    default: DBGBREAK;              return "!!eResType::ERR";
    case eResType::__DO__NOT__USE:  return "!!eResType::__DO__NOT__USE";
    case eResType::__FINISHED:      return "!!eResType::__FINISHED";
    case eResType::gosasset_d:      return "gosasset_d";
    case eResType::shader_txt:      return "shader_txt";
    case eResType::image:           return "image";
	}
}

//***********************************
const char* asset2::enumToString (eAssetType s)
{
	switch (s)
	{
    default: DBGBREAK;                  return "!!eAssetType::ERR";
    case eAssetType::vtx_shader:        return "vtx_shader";
    case eAssetType::pxl_shader:        return "pxl_shader";
    case eAssetType::pipe:              return "pipe";
    case eAssetType::tex2D:             return "tex2D";
    case eAssetType::shape:             return "shape";
	}
}

//***********************************
const char* asset2::enumToString (eBuildResult s)
{
	switch (s)
	{
    default: DBGBREAK;                      return "!!eBuildResult::ERR";
    case eBuildResult::just_built:          return "just_built";
    case eBuildResult::was_already_built:   return "was_already_built";
    case eBuildResult::error:               return "error";
	}
}

//************************************
static bool asset2_create_emptyDB (const char *dbFile, DBHandle &db)
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

            sprintf_s (s, sizeof(s), "INSERT INTO version (ID,ver) VALUES(1,%d)", GOS_ASSET2__DB_VERSION);
            if (!db::exec (db, s))
                break;


            //table: GOS_ASSET2__TABLE_RES
            sprintf_s (s, sizeof(s), "CREATE TABLE " GOS_ASSET2__TABLE_RES " (\
UID UNSIGNED INT8 NOT NULL PRIMARY KEY,\
lastTimeMod UNSIGNED INT8 NOT NULL DEFAULT 0,\
type UNSIGNED INT1 NOT NULL,\
abspath VARCHAR(512) NOT NULL)\
");
            if (!db::exec (db, s))
                break;
                

            //table: GOS_ASSET2__TABLE_DEPENDS
            sprintf_s (s, sizeof(s), "CREATE TABLE " GOS_ASSET2__TABLE_DEPENDS " (\
UID UNSIGNED INT8 NOT NULL,\
childUID UNSIGNED INT8 NOT NULL,\
PRIMARY KEY('UID','childUID'))");
        if (!db::exec (db, s))
                break;


            //table: GOS_ASSET__TABLE_RUNTIME_NAME
            sprintf_s (s, sizeof(s), "CREATE TABLE " GOS_ASSET2__TABLE_RUNTIME_NAME " (\
name VARCHAR(64) NOT NULL PRIMARY KEY,\
assetUID UNSIGNED INT8 NOT NULL\
)");
            if (!db::exec (db, s))
                break;


            //table: TABLE__ASSET_LIST
            sprintf_s (s, sizeof(s), "CREATE TABLE " GOS_ASSET2__TABLE_ASSET_LIST " (\
UID UNSIGNED INT8 NOT NULL PRIMARY KEY,\
lastTimeBuilt UNSIGNED INT8 NOT NULL,\
type UNSIGNED INT1 NOT NULL,\
src VARCHAR(512) NOT NULL)\
");
            if (!db::exec (db, s))
                break;

            //table: GOS_ASSET__TABLE_DEPENDS_RUNTIME 
            sprintf_s (s, sizeof(s), "CREATE TABLE " GOS_ASSET2__TABLE_DEPENDS_RUNTIME " (\
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
static bool asset2_open_or_create_DB (DBHandle &db, const char *baseFolder, const char *dbName)
{
    assert (NULL != baseFolder);
    assert (NULL != dbName);

    char fullDBFilePathAndName[1024];
    sprintf_s (fullDBFilePathAndName, sizeof(fullDBFilePathAndName), "%s/%s", baseFolder, dbName);

    if (!fs::fileExists(fullDBFilePathAndName))
    {
        if (!asset2_create_emptyDB(fullDBFilePathAndName, db))
            return false;
    }
    else
    {
        if (!db::open (fullDBFilePathAndName, &db))
        {
            gos::logger::err ("asset2_open_or_create_DB(): can't open db at %s\n", fullDBFilePathAndName);
            return false;
        }

        //verifico che la versione sia corretta
        db::RST rst;
        if (db::query (db, "SELECT ver FROM version WHERE ID=1", &rst))
        {
            if (rst.fetchRow())
            {
                if (GOS_ASSET2__DB_VERSION != rst.getValAsU32(0))
                {
                    gos::logger::err ("asset2::asset_openDB(): wrong DB version, expected ver=%d\n", GOS_ASSET2__DB_VERSION);
                    return false;
                }
            }
        }
    }

    return true;
}

//*********************************************** 
bool asset2::dbcontext_open (const char *baseFolder, DBContext *out)
{
    return dbcontext_open_ex (baseFolder, "assets2.sqlite3", out);
}

//*******************************************************
bool asset2::dbcontext_open_ex (const char *baseFolderIN, const char *dbName, DBContext *out)
{
    assert (NULL != baseFolderIN);
    assert (NULL != dbName);
    assert (NULL != out);

    if (out->isValid())
        return false;

    char baseFolder[1024];
    fs::resolvePath (baseFolderIN, baseFolder, sizeof(baseFolder));
    

    //apre il db o lo crea se non esiste gia'
    if (!asset2_open_or_create_DB(out->db, baseFolder, dbName))
        return false;

    out->dbName = string::utf8::allocStr (gos::getSysHeapAllocator(), dbName);
    out->baseFolder = string::utf8::allocStr (gos::getSysHeapAllocator(), baseFolder);
  
    char s[1024];
    sprintf_s (s, sizeof(s), "%s/asset_bin", baseFolder);
    out->folder_assets_bin = string::utf8::allocStr (gos::getSysHeapAllocator(), s);
    fs::folderCreate (s);

    sprintf_s (s, sizeof(s), "%s/asset_src", baseFolder);
    out->folder_assets_src = string::utf8::allocStr (gos::getSysHeapAllocator(), s);
    fs::folderCreate (s);
    
    
    return true; 
}

//*********************************************** 
void asset2::dbcontext_close (DBContext &ctx)
{
    if (!ctx.isValid())
        return;

    GOSFREE_AND_NULL (gos::getSysHeapAllocator(), ctx.baseFolder);
    GOSFREE_AND_NULL (gos::getSysHeapAllocator(), ctx.dbName);
    GOSFREE_AND_NULL (gos::getSysHeapAllocator(), ctx.folder_assets_bin);
    GOSFREE_AND_NULL (gos::getSysHeapAllocator(), ctx.folder_assets_src);
    db::close (ctx.db);
}



//********************************************************** 
bool asset2::res_createUID (eResType resTypeIN, const char *absFilenameIN, asset2::UID *out)
{
    assert (NULL != absFilenameIN);
    assert (NULL != out);
    assert (fs::isPathAbsolute(absFilenameIN));

    char s[128];
    sprintf_s (s, sizeof(s), "%02d_%s", static_cast<u8>(resTypeIN), absFilenameIN);

    //crc del nome
    out->_uid = utils::crc32(s, static_cast<u32>(strlen(s)));

    //uso il terzo byte MSB per metterci il resType
    u64 resourceType = static_cast<u64>(resTypeIN);
    resourceType <<= 40;
    out->_uid |= resourceType;

    return true;
}

//********************************************************** 
bool asset2::res_insert (DBContext &ctx, eResType resTypeIN, const char *absFilenameIN, u64 lastTimeMod, UID *out_CAN_BE_NULL_uid)
{
    if (!ctx.isValid())
    {
        logger::err ("asset2::res_insert('%s') => invalid ctx\n",  absFilenameIN);
        return false;
    }

    asset2::UID uid;
    res_createUID (resTypeIN, absFilenameIN, &uid);
    if (NULL != out_CAN_BE_NULL_uid)
        *out_CAN_BE_NULL_uid = uid;

    char s[512];
    sprintf_s (s, sizeof(s), "INSERT INTO " GOS_ASSET2__TABLE_RES " (UID,lastTimeMod,type,abspath) VALUES(%" PRIu64 ",%" PRIu64 ",%d,'%s')", 
                    uid._uid, 
                    lastTimeMod, 
                    static_cast<u8>(resTypeIN),
                    absFilenameIN);
    if (!db::exec (ctx.db, s))
    {
        logger::err ("asset2::res_insert('%s') => error inserting into table\n",  absFilenameIN);
        return false;
    }

    return true;
}

//********************************************************** 
bool asset2::res_update (DBContext &ctx, UID uid, u64 lastTimeMod)
{
    assert (uid.isAResource());

    if (!ctx.isValid())
    {
        logger::err ("asset2::res_update(%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    char s[512];
    sprintf_s (s, sizeof(s), "UPDATE " GOS_ASSET2__TABLE_RES " SET lastTimeMod=%" PRIu64 " WHERE UID=%" PRIu64 ";", lastTimeMod, uid._uid);
    if (!db::exec (ctx.db, s))
    {
        logger::err ("asset2::res_update(%" PRIu64 ") => error updating table\n",  uid._uid);
        return false;
    }

    return true;
}

//********************************************************** 
bool asset2::res_exists (DBContext &ctx, eResType resType, const char *absFilenameIN, UID *out_CAN_BE_NULL_uid)
{
    if (!ctx.isValid())
    {
        logger::err ("asset2::res_exists(\"%s\") => invalid ctx\n", absFilenameIN);
        return false;
    }

    db::RST rst;
    char s[256];
    
    sprintf_s (s, sizeof(s), "SELECT UID FROM " GOS_ASSET2__TABLE_RES " WHERE type=%d AND abspath='%s'", static_cast<u8>(resType), absFilenameIN);
    if (!db::query (ctx.db, s, &rst)) return false;
    if (rst.fetchRow())
    {
        if (NULL != out_CAN_BE_NULL_uid)
            out_CAN_BE_NULL_uid->_uid = rst.getValAsU64(0);
        return true;
    }

    return false;
}

//********************************************************** 
bool asset2::res_get_info (DBContext &ctx, UID uid, char *out_CAN_BE_NULL_abspath, u32 sizeof_outabspath, eResType *out_CAN_BE_NULL_resType, u64 *out_CAN_BE_NULL_lastTimeMod)
{
    assert (uid.isAResource());

    if (!ctx.isValid())
    {
        logger::err ("asset2::res_get_info(%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    db::RST rst;
    char s[128];
    
    sprintf_s (s, sizeof(s), "SELECT lastTimeMod,type,abspath FROM " GOS_ASSET2__TABLE_RES " WHERE UID=%" PRIu64 "", uid._uid);
    if (!db::query (ctx.db, s, &rst)) return false;
    if (rst.fetchRow())
    {
        if (NULL != out_CAN_BE_NULL_lastTimeMod)    *out_CAN_BE_NULL_lastTimeMod = rst.getValAsU64(0);
        if (NULL != out_CAN_BE_NULL_resType)        *out_CAN_BE_NULL_resType = static_cast<eResType>(rst.getValAsU8(1));
        if (NULL != out_CAN_BE_NULL_abspath)        sprintf_s (out_CAN_BE_NULL_abspath, sizeof_outabspath, "%s", rst.getVal(2));

        return true;
    }

    return false;
}

//********************************************************** 
bool asset2::res_delete (DBContext &ctx, const UID &uid)
{
    assert (uid.isAResource());

    if (!ctx.isValid())
    {
        logger::err ("res_delete (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    char s[256];
    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET2__TABLE_RES " WHERE UID=%" PRIu64 "", uid._uid);
    db::exec (ctx.db, s);

    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET2__TABLE_DEPENDS " WHERE UID=%" PRIu64 "", uid._uid);
    db::exec (ctx.db, s);

    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET2__TABLE_DEPENDS " WHERE childUID=%" PRIu64 "", uid._uid);
    db::exec (ctx.db, s);

    return true;
}



//*******************************************************
bool asset2::rtname_exists (DBContext &ctx, const char *runtimeName, UID *out_assetUID)
{
    assert (NULL != out_assetUID);
    out_assetUID->setInvalid();

    if (!ctx.isValid())
    {
        logger::err ("rtname_exists(\"%s\") => invalid ctx\n", runtimeName);
        return false;
    }

    db::RST rst;
    char s[256];
    
    sprintf_s (s, sizeof(s), "SELECT assetUID FROM " GOS_ASSET2__TABLE_RUNTIME_NAME " WHERE name='%s'", runtimeName);
    if (!db::query (ctx.db, s, &rst)) return false;
    if (rst.fetchRow())
    {
        out_assetUID->_uid = rst.getValAsU64(0);
        return true;
    }

    return false;
}

//*******************************************************
bool asset2::rtname_insert (DBContext &ctx, const char *runtimeName, UID assetUID)
{
    assert (assetUID.isAnAsset());

    if (!ctx.isValid())
    {
        logger::err ("rtname_insert(\"%s\", %" PRIu64 ") => invalid ctx\n", runtimeName, assetUID._uid);
        return false;
    }

    char s[256];
    //non esisteva, lo aggiungo
    sprintf_s (s, sizeof(s), "INSERT INTO " GOS_ASSET2__TABLE_RUNTIME_NAME " (name,assetUID) VALUES('%s', %" PRIu64 ")", runtimeName, assetUID._uid);
    if (db::exec (ctx.db, s))
        return true;

    logger::err ("rtname_insert(\"%s\", %" PRIu64 ") => error inserting into table\n", runtimeName, assetUID._uid);
    return false;
}


//*******************************************************
void asset2::asset_manufacture_fullFilename (const DBContext &ctx, UID uid, char *out, u32 sizeof_out)
{
    sprintf_s (out, sizeof_out, "%s/%016" PRIX64 ".gosasset", ctx.folder_assets_bin, uid._uid);
}

//*******************************************************
bool asset2::asset_createUID (eAssetType assTypeIN, u8 asset_depth, const void *buffer, u32 sizeof_buffer, UID *out)
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
bool asset2::asset_insert (DBContext &ctx, UID uid, eAssetType assType, u64 lastTimeBuilt, const char *srcAbsFilename)
{
    assert (uid.isAnAsset());
    assert (NULL != srcAbsFilename);

    if (u64MAX == lastTimeBuilt)
    {
        gos::DateTime dt;
        dt.setNow_UTC();
        lastTimeBuilt = dt.getAsNiceU64();
    }

    if (!ctx.isValid())
    {
        logger::err ("asset_insert (%" PRIu64 ", %d, %" PRIu64 ", '%s') => invalid ctx\n",  uid._uid, assType, lastTimeBuilt, srcAbsFilename);
        return false;
    }

    db::RST rst;
    char s[256];
    sprintf_s (s, sizeof(s), "INSERT INTO " GOS_ASSET2__TABLE_ASSET_LIST " (UID,lastTimeBuilt,type,src) VALUES(%" PRIu64 ",%" PRIu64 ",%d,'%s')", uid._uid, lastTimeBuilt, static_cast<u8>(assType), srcAbsFilename);
    if (!db::exec (ctx.db, s))
    {
        logger::err ("asset_insert(%" PRIu64 ", %d, %" PRIu64 ", '%s') => error inserting into table\n", uid._uid, assType, lastTimeBuilt, srcAbsFilename);
        return false;
    }

    return true;
}

//*******************************************************
bool asset2::asset_get_info (DBContext &ctx, UID uid, char *out_CAN_BE_NULL_src, u32 sizeof_outsrc, eAssetType *out_CAN_BE_NULL_assetType, u64 *out_CAN_BE_NULL_lastTimeBuilt)
{
    assert (uid.isAnAsset());

    if (!ctx.isValid())
    {
        logger::err ("asset2::asset_get_info(%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    db::RST rst;
    char s[128];
    

    sprintf_s (s, sizeof(s), "SELECT lastTimeBuilt,type,src FROM " GOS_ASSET2__TABLE_ASSET_LIST " WHERE UID=%" PRIu64 "", uid._uid);
    if (!db::query (ctx.db, s, &rst)) return false;
    if (rst.fetchRow())
    {
        if (NULL != out_CAN_BE_NULL_lastTimeBuilt)      *out_CAN_BE_NULL_lastTimeBuilt = rst.getValAsU64(0);
        if (NULL != out_CAN_BE_NULL_assetType)          *out_CAN_BE_NULL_assetType = static_cast<eAssetType>(rst.getValAsU8(1));
        if (NULL != out_CAN_BE_NULL_src)                sprintf_s (out_CAN_BE_NULL_src, sizeof_outsrc, "%s", rst.getVal(2));

        return true;
    }

    return false;
}
//*******************************************************
u64 asset2::asset_query_lastTimeBuilt (DBContext &ctx, UID uid)
{
    assert (uid.isAnAsset());

    if (!ctx.isValid())
    {
        logger::err ("asset_query_lastTimeBuilt (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return 0;
    }

    db::RST rst;
    char s[256];
    sprintf_s (s, sizeof(s), "SELECT lastTimeBuilt FROM " GOS_ASSET2__TABLE_ASSET_LIST " WHERE UID=%" PRIu64 "", uid._uid);
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
bool asset2::asset_get_runtime_dependecies_list (DBContext &ctx, UID uid, bool bClearListOnStart, FastUIDList *out)
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
    sprintf_s (s, sizeof(s), "SELECT childUID FROM " GOS_ASSET2__TABLE_DEPENDS_RUNTIME " WHERE UID=%" PRIu64 " ORDER BY childDepth ASC", uid._uid);
    if (!db::query (ctx.db, s, &rst))
    {
        logger::err ("asset_get_runtime_dependecies_list (%" PRIu64 ") => error querying\n",  uid._uid);
        return false;
    }

    while (rst.fetchRow())
    {
        UID childUID;
        childUID._uid = rst.getValAsU64(0);
        out->append (childUID);
    }

    return true;
}

//********************************************************** 
bool asset2::asset_delete (DBContext &ctx, const UID &uid)
{
    assert (uid.isAnAsset());

    if (!ctx.isValid())
    {
        logger::err ("asset_delete (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    char s[256];
    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET2__TABLE_ASSET_LIST " WHERE UID=%" PRIu64 "", uid._uid);
    db::exec (ctx.db, s);

    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET2__TABLE_DEPENDS " WHERE UID=%" PRIu64 "", uid._uid);
    db::exec (ctx.db, s);

    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET2__TABLE_DEPENDS " WHERE childUID=%" PRIu64 "", uid._uid);
    db::exec (ctx.db, s);

    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET2__TABLE_DEPENDS_RUNTIME " WHERE UID=%" PRIu64 "", uid._uid);
    db::exec (ctx.db, s);

    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET2__TABLE_DEPENDS_RUNTIME " WHERE childUID=%" PRIu64 "", uid._uid);
    db::exec (ctx.db, s);

    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET2__TABLE_RUNTIME_NAME " WHERE assetUID=%" PRIu64 "", uid._uid);
    db::exec (ctx.db, s);
    

    asset_manufacture_fullFilename (ctx, uid, s, sizeof(s));
    fs::fileDelete(s);

    //gli shader sono buildati anche con la versione "d"
    if (uid.isAnAssetOfType(eAssetType::vtx_shader) || uid.isAnAssetOfType(eAssetType::pxl_shader))
    {
        strcat_s (s, sizeof(s), "d");
        fs::fileDelete(s);
    }
    return true;
}


//*******************************************************
bool asset2::dependency_get_dependecies_list (DBContext &ctx, UID uid, bool bClearListOnStart, UniqueUIDList *out)
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
        out->insertIfNotExists (childUID);
    }

    rst.rewind();
    while (rst.fetchRow())
    {
        UID childUID;
        childUID._uid = rst.getValAsU64(0);
        if (!dependency_get_dependecies_list (ctx, childUID, false, out))
            return false;
    }
    return true;
}

//********************************************************** 
bool asset2::dependency_get_requireBy_list (DBContext &ctx, const asset2::UID &uid, bool bClearListOnStart, asset2::UniqueUIDList *out)
{
    assert (NULL != out);

    if (bClearListOnStart)
        out->reset();

    if (!ctx.isValid())
    {
        logger::err ("dependency_get_requireBy_list (%" PRIu64 ") => invalid ctx\n",  uid._uid);
        return false;
    }

    db::RST rst;
    char s[256];
    sprintf_s (s, sizeof(s), "SELECT UID FROM " GOS_ASSET2__TABLE_DEPENDS " WHERE childUID=%" PRIu64 "", uid._uid);
    if (!db::query (ctx.db, s, &rst))
    {
        logger::err ("dependency_get_requireBy_list (%" PRIu64 ") => error querying\n",  uid._uid);
        return false;
    }

    while (rst.fetchRow())
    {
        UID childUID;
        childUID._uid = rst.getValAsU64(0);
        out->insertIfNotExists (childUID);

        if (!dependency_get_requireBy_list (ctx, childUID, false, out))
            return false;
    }
    return true;
}

//********************************************************** 
bool asset2::dependency_exists (DBContext &ctx, UID father, UID child)
{
    assert (father.isValid());
    assert (child.isValid());

    if (!ctx.isValid())
    {
        logger::err ("asset2::dependency_exists(%" PRIu64 ",%" PRIu64 ") => invalid ctx\n", father._uid, child._uid);
        return false;
    }

    db::RST rst;
    char s[128];
    sprintf_s (s, sizeof(s), "SELECT UID FROM " GOS_ASSET2__TABLE_DEPENDS " WHERE UID=%" PRIu64 " AND childUID=%" PRIu64 ";", father._uid, child._uid);
    if (!db::query (ctx.db, s, &rst))
    {
        logger::err ("dependency_exists (%" PRIu64 ",%" PRIu64 ") => error querying\n", father._uid, child._uid);
        return false;
    }

    return rst.fetchRow();

}

//********************************************************** 
bool asset2::dependency_add (DBContext &ctx, UID father, UID child)
{
    assert (father.isValid());
    assert (child.isValid());

    if (!ctx.isValid())
    {
        logger::err ("asset2::dependency_add(%" PRIu64 ",%" PRIu64 ") => invalid ctx\n", father._uid, child._uid);
        return false;
    }

    char s[128];
    sprintf_s (s, sizeof(s), "INSERT INTO " GOS_ASSET2__TABLE_DEPENDS " (UID,childUID) VALUES(%" PRIu64 ",%" PRIu64 ");", father._uid, child._uid);
    return db::exec (ctx.db, s);
}

//*******************************************************
bool asset2::dependencyRT_add (DBContext &ctx, UID uid_padre, UID uid_figlio, u8 depth_figlio)
{
    if (!ctx.isValid())
    {
        logger::err ("dependencyRT_add(%016" PRIX64 ",%016" PRIX64 ") => invalid ctx\n", uid_padre._uid, uid_figlio._uid);
        return false;
    }

    char s[128];
    sprintf_s (s, sizeof(s), "INSERT INTO " GOS_ASSET2__TABLE_DEPENDS_RUNTIME " (UID,childUID,childDepth) VALUES(%" PRIu64 ",%" PRIu64 ",%d)", uid_padre._uid, uid_figlio._uid, depth_figlio);
    if (db::exec (ctx.db, s))
        return true;

    logger::err ("dependencyRT_add(%016" PRIX64 ",%016" PRIX64 ") => error inserting into table\n", uid_padre._uid, uid_figlio._uid);
    return false;
}
