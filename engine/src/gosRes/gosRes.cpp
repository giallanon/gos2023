#include "gosRes.h"
#include "gos.h"
#include "gosUtils.h"
#include "string/gosUTF8String.h"
#include "gosRST.h"

using namespace gos;

//*******************************************************
bool res::create_folder_structure (const char *baseFolder)
{
    char s[1024];

    sprintf_s (s, sizeof(s), "%s/compiled", baseFolder);
    if (!fs::folderCreate (s))
        return false;

    sprintf_s (s, sizeof(s), "%s/src", baseFolder);
    if (!fs::folderCreate (s))
        return false;

    sprintf_s (s, sizeof(s), "%s/raw", baseFolder);
    if (!fs::folderCreate (s))
        return false;

    sprintf_s (s, sizeof(s), "%s/raw/shaders", baseFolder);
    if (!fs::folderCreate (s))
        return false;

    sprintf_s (s, sizeof(s), "%s/raw/images", baseFolder);
    if (!fs::folderCreate (s))
        return false;


    return true;
}

//*******************************************************
u32 res::calc_resUID (eResType resType, const void *bufferIN, u32 sizeof_bufferIN)
{
    assert (NULL != bufferIN);
    assert (sizeof_bufferIN > 0);

    u8 *buffer = GOSALLOC_SCRAPT(u8*, sizeof_bufferIN + 4);
    buffer[0] = static_cast<u8>(resType);
    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0;
    memcpy (&buffer[4], bufferIN, sizeof_bufferIN);
    const u32 resID = utils::crc32(buffer, sizeof_bufferIN +4);
    GOSFREE_SCRAP(buffer);

    return resID;
}

//********************************************
void res::manufacture_compiled_fullFilePathAndName (const char *baseFolder, u32 resUID, char *out, u32 sizeof_out)
{
    sprintf_s (out, sizeof_out, "%s/compiled/%08X.gosres", baseFolder, resUID);
}

//********************************************
bool res::shader_compile (const char *shaderSRCFile, const char *shaderStage, const char *spaceSeparateDefineList, const char *shaderDSTFile, bool bIncludeDebugInfo)
{
    //se esistono delle define da passare al compilatore...
    char defineList[2048];
    memset (defineList, 0, sizeof(defineList));
    if (NULL != spaceSeparateDefineList)
    {
        string::utf8::StringListParser parser;
        parser.toStart (spaceSeparateDefineList, ' ');
        
        char def[256];
        while (parser.next (def, sizeof(def)))
        {
            strcat_s (defineList, sizeof(defineList), "-D");
            strcat_s (defineList, sizeof(defineList), def);
            strcat_s (defineList, sizeof(defineList), " ");
        }
    }

    char opt_includeDebugInfo[4];
    if (bIncludeDebugInfo)
        sprintf_s (opt_includeDebugInfo, sizeof(opt_includeDebugInfo), "-g");
    else
        opt_includeDebugInfo[0] = 0x00;

    //glslc -fshader-stage=vert --target-env=vulkan1.3 lineRenderer.vert.shader -g -O -o lineRenderer.vert.spv
    char cmd[1024];
    sprintf_s (cmd, sizeof(cmd), "glslc -fshader-stage=%s --target-env=vulkan1.3 %s %s %s -O -o %s 2>&1",  shaderStage, defineList, shaderSRCFile, opt_includeDebugInfo, shaderDSTFile);
    gos::logger::log ("%s\n", cmd);

    char *result;
    u32 len;
    if (!gos::runShellScriptAndStoreResult (cmd, gos::getScrapAllocator(), &result, &len))
        return false;

    if (NULL == result)
        return true;

    //c'e' stato qualche errore di compilazione
    gos::logger::err ("ERR => %s\n", result);
    GOSFREE_SCRAP(result);
    return false;
}

//********************************************
bool res::find_resUID (DBHandle &db, const char *runtimeResName, u32 *out_resID)
{
    char sql[256];
    sprintf_s (sql, sizeof(sql), "SELECT resID FROM rtname WHERE runtimeName='%s'", runtimeResName);

    db::RST rst;
    if (db::query (db, sql, &rst))
    {
        if (rst.fetchRow())
        {
            *out_resID = rst.getColValueAsU32(0);
            return true;
        }
    }

    return false;
}

//********************************************
bool res::need_rebuild (sBuilderSession &session, u32 resUID, u64 lastTimeIniSectionWasUpdate)
{
    char sql[128];
    sprintf_s (sql, sizeof(sql), "SELECT lastTimeBuilt FROM res WHERE resID=%d", resUID);

    db::RST rst;
    if (!db::query (session.db, sql, &rst))
        return true;

    if (!rst.fetchRow())
        return true;
    
    if (lastTimeIniSectionWasUpdate > rst.getColValueAsU64(0))
        return true;

    return false;
}

//********************************************
void res::onResourceBuilt (sBuilderSession &session, u32 resUID, eResType resType, bool bJustRebuilt, const char *runtimeName)
{
    if (!db::transaction_begin (session.db))
    {
        DBGBREAK;
        return;
    }

    char sql[256];
    db::RST rst;

    //aggiunge resUID alla tabella res se non esiste gia'
    sprintf_s (sql, sizeof(sql), "SELECT resID FROM res WHERE resID=%d", resUID);
    if (!db::query (session.db, sql, &rst))
    {
        DBGBREAK;
        return;
    }
    if (!rst.fetchRow())
    {
        sprintf_s (sql, sizeof(sql), "INSERT INTO res (resID,lastTimeBuilt,resType) VALUES(%d,%" PRIu64 ",%d)",
            resUID, 
            session.timestamp,
            static_cast<u8>(resType));
        db::exec (session.db, sql);
    }

    //aggiorna il lastTimeBuilt se necessario
    if (bJustRebuilt)
    {
        sprintf_s (sql, sizeof(sql), "UPDATE res SET lastTimeBuilt=%" PRIu64 " WHERE resID=%d", session.timestamp, resUID);
        db::exec (session.db, sql);
    }


    //aggiunge runtime-name se necessario
    sprintf_s (sql, sizeof(sql), "SELECT resID FROM rtname WHERE runtimeName='%s' AND resID=%d", runtimeName, resUID);
    if (!db::query (session.db, sql, &rst))
    {
        DBGBREAK;
        db::transaction_rollback (session.db);
        return;
    }
    if (!rst.fetchRow())
    {
        sprintf_s (sql, sizeof(sql), "INSERT INTO rtname (runtimeName,resID) VALUES('%s',%d)", runtimeName, resUID);
        if (!db::exec (session.db, sql))
        {
            gos::logger::err ("The runtimeName '%s' with resID %010u can't be added because the same runtimeName is already used by another resource.\n", runtimeName, resUID);
            db::transaction_rollback (session.db);
            return;
        }

    }



    if (!db::transaction_commit (session.db))
    {
        DBGBREAK;
        return;
    }

}
