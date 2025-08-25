#include "gosResShaderBuilder.h"
#include "gosRes.h"
#include "gos.h"

using namespace gos;
using namespace gos::res;

//************************************
Builder::Builder()
{
    localAllocator = gos::getSysHeapAllocator();

    baseFolder = NULL;
    builderList.setup (localAllocator, sizeof(IResBuilder*) * 32);

    addResBuilder<VtxShaderBuilder>();
    addResBuilder<PxlShaderBuilder>();
}

//************************************
void Builder::priv_free()
{
    db::close (db);

    for (u32 i=0; i<builderList.getNElem(); i++)
    {
        IResBuilder *b = builderList[i];
        GOSDELETE(localAllocator, b);
    }
    builderList.unsetup();

    if (NULL != baseFolder)
    {
        GOSFREE(localAllocator, baseFolder);
        baseFolder = NULL;
    }
}

//************************************
bool Builder::priv_createEmptyDB (const char *dbFile)
{
    if (!db::open (dbFile, &db))
        return false;

    char s[1024];

    //table: version
    sprintf_s (s, sizeof(s), "CREATE TABLE version (ID INTEGER NOT NULL DEFAULT 1 UNIQUE PRIMARY KEY, ver UNSIGNED INT1 NOT NULL DEFAULT 1)");
    if (!db::exec (db, s))
        return false;

    sprintf_s (s, sizeof(s), "INSERT INTO version (ID,ver) VALUES(1,%d)", DB__VER);
    if (!db::exec (db, s))
        return false;


    //table: resname
    sprintf_s (s, sizeof(s), "CREATE TABLE rtname (\
runtimeName VARCHAR(64) NOT NULL UNIQUE PRIMARY KEY,\
resID UNSIGNED INT4 NOT NULL\
)");
    if (!db::exec (db, s))
        return false;

    sprintf_s (s, sizeof(s), "CREATE TABLE res (\
resID UNSIGNED INT4 NOT NULL UNIQUE PRIMARY KEY,\
lastTimeBuilt UNSIGNED INT8 NOT NULL,\
resType UNSIGNED INT1 NOT NULL)\
");
    if (!db::exec (db, s))
        return false;
        

    return true;
}

//************************************
bool Builder::open (const char *baseFolderIN)
{
    if (NULL != baseFolder)
        return false;

    char s[1024];
    fs::resolvePath (baseFolderIN, s, sizeof(s));
    baseFolder = string::utf8::allocStr (localAllocator, s);

    //crea la struttura di cartelle se non esiste gia'
    res::create_folder_structure (baseFolder);

    return priv_openDB();
}

//************************************
bool Builder::priv_openDB()
{
    if (NULL == baseFolder)
        return false;

    //apre il DB delle risorse (o lo crea se non esiste gia')
    char s[1024];
    sprintf_s (s, sizeof(s), "%s/res.db3", baseFolder);
    if (!fs::fileExists(s))
    {
        if (!priv_createEmptyDB(s))
        {
            fs::fileDelete(s);
            return false;
        }
    }
    else
    {
        if (!db::open (s, &db))
            return false;
    }


    //recupero la versione attuale del DB
    db::RST rst;
    if (!db::query (db, "SELECT ver FROM version WHERE ID=1", &rst))
    {
        logger::err ("Builder::open => can't query db version\n");
        return false;
    }
    if (rst.getNumCols() == 0)
    {
        logger::err ("Builder::open => can't query db version (2)\n");
        return false;
    }

    rst.fetchRow();
    {
        const u32 dbVer = rst.getColValueAsU32(0);
        if (dbVer != DB__VER)
            priv_updateDBToCurrentDBVer();
    }

    return true;
}

//************************************
void Builder::priv_updateDBToCurrentDBVer()
{
    //TODO
    return;
}

//************************************
bool Builder::priv_addResBuilder (IResBuilder *builder)
{
    for (u32 i=0; i<builderList.getNElem(); i++)
    {
        if (builderList(i)->getResType() == builder->getResType())
        {
            gos::logger::err ("Builder::priv_addResBuilder() => a builder for '%s' already exists\n", utils::enumToString(builder->getResType()));
            return false;
        }    
    }

    builderList.append (builder);
    return true;
}

//************************************
void Builder::priv_buildFolder (sBuilderSession &session, const char *folder)
{
    gos::FileFind ff;
    if (fs::findFirst (&ff, folder, "*.gosresd"))
    {
        do
        {
            char s[1024];

            const char *filename = fs::findGetFileName(ff);
            if (fs::findIsDirectory(ff))
            {
                if (filename[0] == '.')
                    continue;
                sprintf_s (s, sizeof(s), "%s/%s", folder, filename);
                priv_buildFolder (session, s);
            }
            else
            {
                sprintf_s (s, sizeof(s), "%s/%s", folder, filename);
                priv_buildFile (session, s);
            }
        } while (fs::findNext(ff));
        
        fs::findClose(ff);
    }
}

//************************************
void Builder::priv_buildFile (sBuilderSession &session, const char *fileFullPathAndName)
{
    gos::logger::log ("Builder => parsing file '%s'\n", fileFullPathAndName);
    gos::logger::incIndent();
    priv_do_buildFile (session, fileFullPathAndName);
    gos::logger::decIndent();
}
void Builder::priv_do_buildFile (sBuilderSession &session, const char *fileFullPathAndName)
{
    IniFile ini;
    if (!ini.loadAndParse (fileFullPathAndName))
    {
        gos::logger::err ("error parsing file\n");
        return;
    }

    gos::DateTime dt;
    fs::fileGetLastTimeModified_UTC (fileFullPathAndName, &dt);
    const u64 lastTimeFileWasUpdated = dt.formatAsU64_yymmddhhmmss();
    gos::logger::log (eTextColor::cyan, "last time modified: %" PRIu64 "\n", lastTimeFileWasUpdated);

    
    for (u32 i=0; i<ini.getNSubsection(); i++)
    {
        gos::IniFileSection *sec = ini.getSubsectionByIndex(i);

        //il nome della sezione indica il tipo di risorsa
        bool bFoundABuilder = false;
        for (u32 i2=0; i2<builderList.getNElem(); i2++)
        {
            const char *resTypeName = utils::enumToString (builderList(i2)->getResType());

            //secion name potrebbe terminare con @index@, cosa che voglio togliere
            //prima di confrontarla con il nome del tipo risorsa
            char secName[128];
            sprintf_s (secName, sizeof(secName), "%s", sec->name.getBuffer());
            u8 ct = 0;
            while (secName[ct])
            {
                if ('@' == secName[ct])
                {
                    secName[ct] = 0;
                    break;
                }
                ct++;
            }

            if (0 == strcmp (secName, resTypeName))
            {
                bFoundABuilder = true;
                gos::logger::log ("found a builder for section <%s>\n", sec->name.getBuffer());
                gos::logger::incIndent();
                builderList[i2]->build (session, sec, lastTimeFileWasUpdated);
                gos::logger::decIndent();
                break;
            }
        }

        if (!bFoundABuilder)
        {
            gos::logger::err ("can't find a builder for section <%s>\n", sec->name.getBuffer());
        }
    }
}

//************************************
void Builder::priv_newSession (sBuilderSession *out) const
{
    assert (NULL != out);
    out->baseFolder = baseFolder;
    out->db = db;

    gos::DateTime dt;
    dt.setNow_UTC();
    out->timestamp = dt.formatAsU64_yymmddhhmmss();  
}

//************************************
void Builder::rebuildAll()
{
    //elimina tutte le risorse compilate
    char s[1024];
    sprintf_s (s, sizeof(s), "%s/compiled", baseFolder);
    fs::folderDeleteAllFileWithJolly  (s, "*.gosres");

    //elimina il db e ne crea uno nuovo
    db::close(db);

    sprintf_s (s, sizeof(s), "%s/res.db3", baseFolder);
    fs::fileDelete(s);
    priv_openDB();

    buildAll();
}

//************************************
void Builder::buildAll ()
{
    sBuilderSession session;
    priv_newSession (&session);

    //scanna la directory <baseFolder>/src alla ricerca di file .gosresd
    char s[1024];
    sprintf_s (s, sizeof(s), "%s/src", baseFolder);
    priv_buildFolder (session, s);
}