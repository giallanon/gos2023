#include "gos.h"
#include "gosAsset2.h"

using namespace gos;

#define GOS_ASSET2__DB_VERSION           1



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

    char s[1024];
    fs::resolvePath (baseFolderIN, s, sizeof(s));
    

    //apre il db o lo crea se non esiste gia'
    if (!asset2_open_or_create_DB(out->db, s, dbName))
        return false;

    out->dbName = string::utf8::allocStr (gos::getSysHeapAllocator(), dbName);

    out->baseFolder = string::utf8::allocStr (gos::getSysHeapAllocator(), s);
   
    
    return true; 
}

//*********************************************** 
void asset2::dbcontext_close (DBContext &ctx)
{
    if (!ctx.isValid())
        return;

    GOSFREE_AND_NULL (gos::getSysHeapAllocator(), ctx.baseFolder);
    GOSFREE_AND_NULL (gos::getSysHeapAllocator(), ctx.dbName);
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