#include "gosAssetBuilder.h"
#include "gos.h"
#include "string/gosStringIncludeDetector.h"
#include "builders/gosAssetBuilder_pipe.h"
#include "builders/gosAssetBuilder_shader.h"
#include "builders/gosAssetBuilder_DEBUG_ASSET.h"
#include "builders/gosAssetBuilder_tex2D.h"

using namespace gos;
using namespace gos::asset;

char Builder::DB_NAME[32] = { "assets.sqlite3" };

//***********************************
const char* Builder::enumToString (const eBuildStatus s)
{
    switch (s)
    {
    default:                        return "!eBuildStatus::invalid value";
    case eBuildStatus::DONT_KNOW:   return "DONT-KNOW";
    case eBuildStatus::NEW:         return "new";
    case eBuildStatus::MODIFIED:    return "modified";
    case eBuildStatus::DELETED:     return "DELETED";
    case eBuildStatus::UNCHANGED:   return "unchanged";
    }
}

//***********************************
Builder::Builder(gos::GPU *gpuIN)
{
    localAllocator = gos::getSysHeapAllocator();
    gpu = gpuIN;
    logger = &loggerNull;
    
    //suppongo un massimo di NUM_MAX_ASSET_BUILDER tipo di asset diversi
    u32 size;
    size = sizeof(BuilderInterface*) * NUM_MAX_ASSET_BUILDER;
    memset (builderList, 0, size);

    size = sizeof(u32) * NUM_MAX_ASSET_BUILDER;
    memset (depthByAssetTypeList, 0xFF, sizeof(depthByAssetTypeList));

    //default builder
    addBuilder<gos::asset::Builder_vtxShader>();
    addBuilder<gos::asset::Builder_pxlShader>();
    addBuilder<gos::asset::Builder_pipe>();
    addBuilder<gos::asset::Builder_DEBUG_ASSET>();
    addBuilder<gos::asset::Builder_tex2D>();

}

//***********************************
Builder::~Builder()
{
    for (u32 i=0; i<NUM_MAX_ASSET_BUILDER; i++)
    {
        if (NULL == builderList[i])
            continue;
        builderList[i]->deinitOnce();
        GOSDELETE(localAllocator, builderList[i]);
    }

   
    priv_closeAllContext();
}

//***********************************
void Builder::priv_closeAllContext()
{
    asset::context_close (ctx);
    asset::context_close (ctx_backup);
    asset::context_close (ctx_sanity);
}

//***********************************
bool Builder::priv_addBuilder (BuilderInterface *builder, u32 asset_depth)
{
    assert (NULL != builder);
    
    const u32 index = static_cast<u8>(builder->getAssType());
    assert (index < NUM_MAX_ASSET_BUILDER);

    if (NULL == builderList[index])
    {
        builderList[index] = builder;
        depthByAssetTypeList[index] = asset_depth;
        builder->initOnce(gpu);
        return true;
    }
    
    logger->err ("asset::Builder::priv_addBuilder() => a builder for res %s already exists\n", asset::enumToString(builder->getAssType()));
    return false;
}

//***********************************
BuilderInterface* Builder::priv_getBuilder (eAssetType assType)
{
    const u32 index = static_cast<u8>(assType);
    assert (index < NUM_MAX_ASSET_BUILDER);
    return builderList[index];
}

//***********************************
void Builder::priv_printResList (const ResList &list) const
{
    for (u32 i=0; i<list.getNElem(); i++)
    {
        eTextColor color = eTextColor::grey;
        switch (list(i).status)
        {
        default:
            color = eTextColor::magenta;
            break;

        case eBuildStatus::NEW:
        case eBuildStatus::MODIFIED:
            color = eTextColor::green;
            break;

        case eBuildStatus::DELETED:
            color = eTextColor::red;
            break;

        case eBuildStatus::UNCHANGED:
            color = eTextColor::grey;
            continue;
            break;
        }

        gos::DateTime dt;
        dt.setFromNiceU64 (list(i).lastTimeModified);

        char lastTimeMod[64];
        dt.formatAs_YYYYMMDDHHMMSS (lastTimeMod, sizeof(lastTimeMod));            

        logger->log (color, "%-10s %016" PRIX64 " %-64s [%-12s] % 20s\n", 
            enumToString(list(i).status),
            list(i).uid._uid, 
            list(i).name, 
            asset::enumToString (list(i).resType), 
            lastTimeMod);
    }
}

//***********************************
bool Builder::priv_fromSectionNameToAssetType (const char *secName, eAssetType *out) const
{
    assert (NULL != secName);
    assert (NULL != out);

    if (secName[0] != '@')
        return false;

    char s[512];
    sprintf_s (s, sizeof(s), "%s", &secName[1]);
    u32 i=0;
    while (s[i])
    {
        if (s[i] == '@')
        {
            s[i] = 0;
            break;
        }
        i++;
    }

    return asset::stringToEnum (s, out);
}

//***********************************
u32 Builder::priv_fromSectionNameToAssetDepthAndType (const char *name, eAssetType *out_assType) const
{
    if (!priv_fromSectionNameToAssetType(name, out_assType))
        return u32MAX;
    return priv_getDepthByAssetType (*out_assType);
}


//***********************************
u32 Builder::debug_sanityCheck__count (db::RST &rst) const
{
    u32 ret = 0;
    while (rst.fetchRow())
        ret++;

    rst.rewind();
    return ret;
}

bool Builder::debug_sanityCheck__compare (db::RST &rst1, db::RST &rst2, u32 rowIndex) const
{
    const char *v1 = rst1.getVal(rowIndex);
    const char *v2 = rst2.getVal(rowIndex);
    if (NULL == v1)
    {
        if (NULL == v2)
            return true;
        return false;
    }
    if (NULL == v2)
        return false;

    return (0 == strcmp (v1, v2));
}

bool Builder::debug_sanityCheck__cmp_table (const char *sql, const char *tableName)
{
    db::RST rst1;
    db::query (ctx.db, sql, &rst1);

    db::RST rst2;
    db::query (ctx_sanity.db, sql, &rst2);

    const u32 n1 = debug_sanityCheck__count (rst1);
    const u32 n2 = debug_sanityCheck__count (rst2);

    if (n1 != n2)
    {
        logger->log ("table '%s' => num record differs (%d vs %d)\n", tableName, n1, n2);
        return false;
    }

    bool ret = true;
    u32 rowNum = 0;
    while (rst1.fetchRow())
    {
        rst2.fetchRow();
        rowNum++;

        for (u32 i=0; i<rst1.getNumCols(); i++)
        {
            if (!debug_sanityCheck__compare (rst1, rst2, i))
            {
                ret = false;
                logger->log ("table '%s', row %d => value for col '%s' differs\n", tableName, rowNum, rst1.getColName(i));
            }
        }
    }
    return ret;
}

void Builder::debug_sanityCheck (const char *baseFolderIN)
{
    /*  in pratica, faccio una copia del DB attuale e poi faccio un rebuildAll, solo che non creo davvero
        i file .gosasset, mi limito a ricreare il DB per poterne poi verificare i dati.
        Dopo il rebuildAll, confronto i 2 DB e vedo che siano uguali
    */
    logger = gos::logger::getSystemLogger();
    logger->log (eTextColor::yellow, "\n\n=== RUNNING SANITY CHECK....\n");
    logger->incIndent();

    const bool ret  = debug_sanityCheck_run (baseFolderIN);
    priv_closeAllContext();
    
    if (ret)
        logger->log (eTextColor::green, "success\n");
    else
        logger->log (eTextColor::red, "FAILED\n");

    logger->decIndent();
}

bool Builder::debug_sanityCheck_run (const char *baseFolderIN)
{
    priv_closeAllContext();

    char dbSRC[1024];
    sprintf_s (dbSRC, sizeof(dbSRC), "%s/%s", baseFolderIN, DB_NAME);
    if (!fs::fileExists(dbSRC))
    {
        logger->err ("debug_sanityCheck => file not found %s\n", dbSRC);
        return false;
    }

    char dbSanity[1024];
    sprintf_s (dbSanity, sizeof(dbSanity), "%s/%s.sanity", baseFolderIN, DB_NAME);
    if (!fs::fileCopy (dbSRC, dbSanity))
    {
        logger->err ("debug_sanityCheck => error while copying '%s' into '%s'\n", dbSRC, dbSanity);
        return false;
    }    

    bool ret = rebuildAll (baseFolderIN, false, false);
    logger = gos::logger::getSystemLogger();

    if (!ret)
    {
        logger->err ("debug_sanityCheck => buildAll failed, restoring old DB\n");

        fs::fileDelete (dbSRC);
        fs::fileCopy (dbSanity, dbSRC);
        fs::fileDelete (dbSanity);
        return false;
    }
    

    //ora faccio un po' di verifiche tra i 2 DB
    char s[256];
    
    sprintf_s (s, sizeof(s), "%s.sanity", DB_NAME);
    asset::context_open_ex (baseFolderIN, s, &ctx_sanity);

    sprintf_s (s, sizeof(s), "%s", DB_NAME);
    asset::context_open_ex (baseFolderIN, s, &ctx);
    {
        sprintf_s (s, sizeof(s), "SELECT UID,type,src FROM " GOS_ASSET__TABLE_ASSET_LIST " ORDER BY UID");
        if (!debug_sanityCheck__cmp_table (s, GOS_ASSET__TABLE_ASSET_LIST))
            ret = false;

        sprintf_s (s, sizeof(s), "SELECT UID,childUID FROM " GOS_ASSET__TABLE_DEPENDS " ORDER BY UID");
        if (!debug_sanityCheck__cmp_table (s, GOS_ASSET__TABLE_DEPENDS))
            ret = false;

        sprintf_s (s, sizeof(s), "SELECT UID,type,name FROM " GOS_ASSET__TABLE_RES_LIST " ORDER BY UID");
        if (!debug_sanityCheck__cmp_table (s, GOS_ASSET__TABLE_RES_LIST))
            ret = false;

        sprintf_s (s, sizeof(s), "SELECT name,assetUID FROM " GOS_ASSET__TABLE_RUNTIME_NAME " ORDER BY name");
        if (!debug_sanityCheck__cmp_table (s, GOS_ASSET__TABLE_RUNTIME_NAME))
            ret = false;
    }

    //chiudo i ctx
    priv_closeAllContext();

    //elimino dbSanity
    if (ret)
        fs::fileDelete (dbSanity);
    
    return ret;
}



//***********************************
bool Builder::rebuildAll (const char *baseFolder, bool bVerbose, bool doCreateAssetsFile)
{
    if (bVerbose)
        logger = gos::logger::getSystemLogger();
    else
        logger = &loggerNull;

    priv_closeAllContext();

    bool ret = true;
    logger->log (eTextColor::white, "REBUILD ALL\n");
    logger->incIndent();
    {
        //elimina il DB
        char s[512];
        sprintf_s (s, sizeof(s), "%s/%s", baseFolder, DB_NAME);
        fs::fileDelete(s);

        //elimina tutti gli asset mai creati
        if (doCreateAssetsFile)
        {
            asset::asset_get_binfolder_name (baseFolder, s, sizeof(s));
            fs::folderDeleteAllFileWithJolly (s, "*.gosasset");
            fs::folderDeleteAllFileWithJolly (s, "*.gosassetd");
            fs::folderDeleteAllFileWithJolly (s, "*.gosasset.reflect");
        }

        //builda
        ret = buildAll(baseFolder, bVerbose, doCreateAssetsFile);
        
    }
    logger->decIndent();

    return ret;
}

//***********************************
bool Builder::buildAll (const char *baseFolder, bool bVerbose, bool doCreateAssetsFile)
{
    if (bVerbose)
        logger = gos::logger::getSystemLogger();
    else
        logger = &loggerNull;

    priv_closeAllContext();
    
    if (!asset::context_open_ex (baseFolder, DB_NAME, &ctx))
    {
        logger->err ("asset::Builder::build() => invalid ctx\n");
        return false;
    }

    //data e ora di build
    char s[1024];
    gos::DateTime dt;
    dt.setNow_UTC();
    this->buildTimeUTC = dt.getAsNiceU64();
    nextTempNameIndex = nextTempSubsectionIndex = 0;

    logger->log (eTextColor::white, "build started...\n");
    logger->incIndent();
        //faccio un backup del DB in caso qualcosa vada male
        asset::context_cloneDB (ctx, ".backup");

        //apro il ctx per il DB di backup, mi serve nella build per printare alcune info
        sprintf_s (s, sizeof(s), "%s.backup", DB_NAME);
        if (!asset::context_open_ex (baseFolder, s, &ctx_backup))
        {
            logger->err ("asset::Builder::build() => error opening backup ctx\n");
            return false;
        }

        //se esiste, elimino il db .failed frutto di una build fallita precedentemente
        sprintf_s (s, sizeof(s), "%s/%s.failed", baseFolder, DB_NAME);
        fs::fileDelete(s);
        

        //build
        const u32 num_errors = priv_do_build(ctx, doCreateAssetsFile);
    logger->decIndent();


    //finito!

    //chiudo il ctx di backup che di sicuro non mi serve piu'
    asset::context_close (ctx_backup);


    //se tutto ok, ho finito per davvero
    if (0 == num_errors)
    {
        asset::context_close (ctx);        

        //elimino il DB di backup visto che e' andato tutto bene
        sprintf_s (s, sizeof(s), "%s/%s.backup", baseFolder, DB_NAME);
        fs::fileDelete(s);
        return true;
    }

    //ci sono stati degli errori, ripristino il vecchio db
    logger->err ("There were errors (%d)\n", num_errors);
    logger->log ("Restoring previous db\n");
    
    //creo una copia del db fallito nel caso qualcuno lo voglia analizzare
    asset::context_cloneDB (ctx, ".failed");
    asset::context_close (ctx);
    
    //elimino il DB visto che ho fallito la build
    sprintf_s (s, sizeof(s), "%s/%s", baseFolder, DB_NAME);
    fs::fileDelete(s);

    //rinomino il db.backup in modo da ripristinare il vecchio db
    sprintf_s (s, sizeof(s), "%s.backup", DB_NAME);
    fs::fileRename (baseFolder, s, DB_NAME);
    return false;    
}

//***********************************
u32 Builder::priv_do_build (Context &ctx, bool doCreateAssetsFile)
{
    u32 num_errors = 0;

    //<deletedAssetList> alla fine del processo di build contiene la lista
    //degli UID degli asset che sono stati eliminati dal DB
    HashedUIDList   deletedAssetList (localAllocator, 256);

    //<scriptToBeRebuilt> lista dei .gosasset_d che devo buildare
    HashedUIDList   toBeRebuiltScriptList (localAllocator, 32);    
    HashedUIDList   deletedScriptList (localAllocator, 15);
    HashedUIDList   deletedResList (localAllocator, 15);

    //elenco delle risorse che sono stati modificate, cancellate o create nuove
    {
        ResList resourceList(localAllocator, 256);
        {
            logger->log (eTextColor::blue, "scanning resource folders...\n");
            logger->incIndent();
            {
                num_errors += priv_collectResInfo (resourceList);
                priv_printResList (resourceList);
            }
            logger->decIndent();
        }

        if (0 != num_errors)
            return num_errors;


        //Esamino la lista delle risorse.
        bool bAnyShaderResourceToProcess = false;
        for (u32 i=0; i<resourceList.getNElem(); i++)
        {   
            switch (resourceList(i).status)
            {
            default:
            case eBuildStatus::DONT_KNOW:
                //questo non deve mai succede
                DBGBREAK;
                break;

            case eBuildStatus::UNCHANGED:
                //non mi interessano, non hanno alcun effetto ai fini della build
                break;

            case eBuildStatus::NEW:
                //devo semplicemente aggiornare il DB con la nuova risorsa
                if (!asset::res_insert (ctx, resourceList(i).uid, resourceList(i).lastTimeModified, resourceList(i).resType, resourceList(i).name))
                    num_errors++;
                else
                {
                    if (eResType::gosasset_d == resourceList(i).resType)
                        toBeRebuiltScriptList.insertIfNotExists (resourceList(i).uid, 0);
                    else if (eResType::shader_txt == resourceList(i).resType)
                        bAnyShaderResourceToProcess = true;
                }
                break;

            case eBuildStatus::DELETED:
                //una risorsa DELETED influenza tutti gli asset che dipendono da lei.
                //Colleziono tutti gli asset UID che dipendono da questa risors.
                //La risorsa la elimino dal DB successivamente
                if (!res_get_requireBy_list (ctx, resourceList(i).uid, false, &deletedAssetList, asset::eFilter::only_assets))
                    num_errors++;
                else
                {
                    if (eResType::gosasset_d == resourceList(i).resType)
                        deletedScriptList.insertIfNotExists (resourceList(i).uid, 0);
                    
                    deletedResList.insertIfNotExists (resourceList(i).uid, 0);

                    if (eResType::shader_txt == resourceList(i).resType)
                        bAnyShaderResourceToProcess = true;

                }
                break;
                
            case eBuildStatus::MODIFIED:
                //una risorsa MODIFIED influenza tutti gli asset che dipendono da lei.
                //Colleziono tutti gli asset UID che dipendono da questa risorsa
                if (!res_get_requireBy_list (ctx, resourceList(i).uid, false, &deletedAssetList, asset::eFilter::only_assets))
                    num_errors++;
                else
                {
                    //aggiorno il lastTimeModified di questa risorsa
                    if (!asset::res_update (ctx, resourceList(i).uid, buildTimeUTC))
                        num_errors++;

                    if (eResType::gosasset_d == resourceList(i).resType)
                        toBeRebuiltScriptList.insertIfNotExists (resourceList(i).uid, 0);
                    else if (eResType::shader_txt == resourceList(i).resType)
                        bAnyShaderResourceToProcess = true;
                }
                break;
            }
        }


        //le risorse shader possono avere delle dipendenze da altre risorse shader dato che ci sono 
        //degli include nei file src
        if (bAnyShaderResourceToProcess)
        {
            //al primo giro gestisco le DELETED, rimuovendole dal DB e 
            //marcando come MODIFIED le risorse che dipendono da lei.
            //In questo modo, al secondo giro, le risorse MODIFIED ricreano la lista
            //di dipendenze e, se non trovano la DELETED, si arrabbiano 
            HashedUIDList tempResList(localAllocator, 32);
            for (u32 i=0; i<resourceList.getNElem(); i++)
            {   
                if (eResType::shader_txt != resourceList(i).resType)
                    continue;

                if (eBuildStatus::DELETED == resourceList(i).status)
                {
                    if (!asset::res_get_requireBy_list (ctx, resourceList(i).uid, true, &tempResList, eFilter::only_resources))
                    {
                        num_errors++;
                        return num_errors;
                    }

                    tempResList.forEach ( [&resourceList] (const asset::UID &key, u64 value)
                    {
                        resourceList.forEach ( [key](u32 index, sResListElem &elem)
                        {
                            if (elem.uid == key && eBuildStatus::UNCHANGED == elem.status)
                            {
                                elem.status = eBuildStatus::MODIFIED;
                                return false;
                            }
                            return true;
                        });
                        
                        return true;
                    });
                }
            }

            for (u32 i=0; i<resourceList.getNElem(); i++)
            {   
                if (eResType::shader_txt != resourceList(i).resType)
                    continue;

                switch (resourceList(i).status)
                {
                default:
                    DBGBREAK;
                    break;

                case eBuildStatus::DELETED:
                case eBuildStatus::UNCHANGED:
                    break;

                case eBuildStatus::NEW:
                    num_errors += priv_shaderRes_add_dependencies (resourceList, i);
                    break;
                    
                case eBuildStatus::MODIFIED:
                    {
                        //prima elimino le attuali dipendenze da altre risorse, e poi le ricalcolo
                        if (!priv_shaderRes_remove_dependencies (resourceList(i).uid))
                            num_errors++;
                        
                        num_errors += priv_shaderRes_add_dependencies (resourceList, i);
                    }
                    break;
                }
            }
        }

    }
    if (0 != num_errors)
        return num_errors;

    //Ho collezionato tutti gli asset che sono stati influenzati dalle modifiche fatte alle risorse.
    //Questi asset vanno eliminati dal DB e fisicamente dal disco per poi, eventualmente, essere rebuildati.
    //Quello che voglio ora quindi e' una lista di gosasset_d che devo rebuildare
    {
        HashedUIDList   tempHashList1 (localAllocator, 256);
        HashedUIDList   tempHashList2 (localAllocator, 256);

        HashedUIDList   *hashList1 = &tempHashList1;
        HashedUIDList   *hashList2 = &tempHashList2;
        hashList1->copyFrom (deletedAssetList);
        deletedAssetList.reset();

        u32 num_deleted = u32MAX;
        while (num_deleted != 0)
        {
            num_deleted = 0;
            hashList2->reset();
            
            auto tempList = hashList1->_queryList();
            const u32 n = tempList->getNElem();
            for (u32 i=0; i<n; i++)
            {
                asset::UID uid;
                uid = tempList->queryElem(i).key;

                //se non e' un asset che ho gia' eliminato in precedenza...
                if (deletedAssetList.insertIfNotExists(uid, 0))
                {
                    num_deleted++;

                    //recpero' l'elenco degli script che servono per buildare questo asset
                    if (!asset::asset_get_script_list (ctx_backup, uid, false, &toBeRebuiltScriptList))
                    {
                        num_errors++;
                        return num_errors;
                    }

                    //elimino l'asset dal DB e dal disco e popolo una nuova lista di asset dipendenti da questo asset.
                    //Questi andranno a loro volta eliminati
                    if (!asset::asset_deleteFromDB (ctx, uid, hashList2, false))
                    {
                        num_errors++;
                        return num_errors;
                    }
                }
            }

            HashedUIDList *swap = hashList1;
            hashList1 = hashList2;
            hashList2 = swap;
        }
    }
    if (0 != num_errors)
        return num_errors;

    //info a video
    logger->log (eTextColor::blue, "List of deleted assets:\n");
    logger->incIndent();
    {
        //le devo prendere dal DB di backup perche' a questo punto gli asset sono stati eliminati
        auto tempList = deletedAssetList._queryList();
        const u32 n = tempList->getNElem();
        for (u32 i=0; i<n; i++)
        {
            asset::UID uid;
            uid = tempList->queryElem(i).key;        

            char s[1024];
            eAssetType assType;
            asset::asset_get_info (ctx_backup, uid, s, sizeof(s), &assType, NULL);
            logger->log (eTextColor::red, "%016" PRIX64 " [%-12s] %s\n", uid._uid, asset::enumToString (assType), s);

            //elimino fisicamente l'asset dal disk
            asset::asset_manufacture_fullFilename (ctx, uid, s, sizeof(s));
            fs::fileDelete(s);

            if (uid.isAnAssetOfType(eAssetType::vtx_shader) || uid.isAnAssetOfType(eAssetType::pxl_shader))
            {
                strcat_s (s, sizeof(s), "d");
                fs::fileDelete(s);
            }
            else if (uid.isAnAssetOfType(eAssetType::pipe))
            {
                strcat_s (s, sizeof(s), ".reflect");
                fs::fileDelete(s);
            }

        }
    }
    logger->decIndent();


    //se c'erano delle risorse DELETED le elimino ora dal DB
    {
        auto tempList = deletedResList._queryList();
        const u32 n = tempList->getNElem();
        for (u32 i=0; i<n; i++)
        {
            asset::UID uid;
            uid = tempList->queryElem(i).key;

            asset::res_delete (ctx, uid);
        }
    }



    //In <toBeRebuiltScriptList> ho una lista di script da rebuildare
    //In <deletedScriptList> ho una lista di script che sono stati eliminati dall'utente.
    //Tolgo da <toBeRebuiltScriptList> tutti i <deletedScriptList> visto che non esistono piu'
    {
        auto tempList = deletedScriptList._queryList();
        const u32 n = tempList->getNElem();
        for (u32 i=0; i<n; i++)
        {
            toBeRebuiltScriptList.remove (tempList->queryElem(i).key);
        }
    }

    /*esplosione dei fine gosres_d
        Prendo tutti i file gosres_d in lista e li esplodo il che vuol dire che creo un nuovo IniFile che contiene tutti gli asset da creare.
        Per ogni asset che ha un sotto-asset, esplodo il file ini tirando fuori la sottosezione
        Alla fine della procudera, ho un IniFile in cui anche gli asset con sottorisorsa sono sempre espressi come asset che si riferiscono ad un runtimeName

        Il file finale lo trovi in /asset/src/__build.gosasset_d

        Ad esempio:
            @pipeline_def : pipe1
            {
                param1: pippo
                param2: pluto

                //questo shader è completamente nuovo, esclusivamente definito qui
                @vtx_shader
                {
                    src: ex5.vert
                }
            }

        diventa:

            @vtx_shader@0@
            {
                src : ex5.vert
                __value : __assname_000000
            }


            @pipeline_def@1@
            {
                __value : pipe1
                param1 : pippo
                param2 : pluto
                @vtx_shader@2@
                {
                    __value : __assname_000000
                }
            }

        Per quei sub-asset che non hanno un vero runtimeName, gliene assegno io uno temporaneo (e' il caso di __assname_000000
    */
    gos::IniFile iniExploded;
    logger->log (eTextColor::blue, "List of script that need rebuild:\n");
    logger->incIndent();
    {
        iniExploded.setup (localAllocator);
        
        char s[1024];
        sprintf_s (s, sizeof(s), "%s/__build.gosasset_d", ctx.folder_assets_src);
        iniExploded.createEmpty (s);

        auto tempList = toBeRebuiltScriptList._queryList();
        const u32 n = tempList->getNElem();
        for (u32 i=0; i<n; i++)
        {
            asset::UID uid;
            uid = tempList->queryElem(i).key;        
        
            char name[128];
            asset::res_get_info (ctx, uid, name, sizeof(name), NULL, NULL);
            logger->log ("%016" PRIX64 " %s\n", uid._uid, name);
            logger->incIndent();
            {
                gos::IniFile ini;
                sprintf_s (s, sizeof(s), "%s/%s", ctx.folder_assets_src, name);
                if (!ini.loadAndParse (s))
                {
                    num_errors++;
                }
                else
                {
                    for (u32 i2=0; i2<ini.getNSubsection(); i2++)
                    {
                        //deve essere di tipo direttiva, altrimenti e' un errore
                        gos::IniFileSection *sub = ini.getSubsectionByIndex(i2);
                        if (sub->name.getBuffer()[0] != '@')
                        {
                            logger->err ("invalid subsection, it must start with @\n");
                            num_errors++;
                        }                
                        else
                        {
                            if (!sub->exists ("__value"))
                            {
                                //gli devo assegnare un __value
                                sprintf_s (s, sizeof(s), "__assname_%06d", nextTempNameIndex++);
                                sub->set ("__value", s);
                            }                        
                            if (!priv_explodeScript_ric (iniExploded.getRoot(), sub, name, uid))
                                num_errors++;
                        }
                    }
                }
            }
            logger->decIndent();
        }

        iniExploded.save();
    }
    logger->decIndent();
    if (0 != num_errors)
        return num_errors;    


    //finalmente posso buildare per davvero!!
    logger->log (eTextColor::blue, "Now building...\n");
    logger->incIndent();
    FastUIDList newlyCreatedAssetList(gos::getSysHeapAllocator(), 256);
    num_errors = priv_build_explodedIniFileInFolder (iniExploded, doCreateAssetsFile, newlyCreatedAssetList);
    logger->decIndent();
    if (0 != num_errors)
        return num_errors;    


    //clean up del DB
    //Elimino tutti i runtimeName che iniziano con __ dato che li ho creati io artificialmente durante il build
    db::exec (ctx.db, "DELETE FROM " GOS_ASSET__TABLE_RUNTIME_NAME " WHERE name LIKE '!_!_%' escape '!'");


    if (deletedAssetList.getNElem() || newlyCreatedAssetList.getNElem())
    {
        logger->log ("Final report\n");
        logger->log ("-----------------------------------\n");
        logger->incIndent();

        auto tempList = deletedAssetList._queryList();
        const u32 n = tempList->getNElem();
        for (u32 i=0; i<n; i++)
        {
            asset::UID uidDeleted;
            uidDeleted = tempList->queryElem(i).key;

            //vediamo se e' stata rebuildata in un assett con lo stesso UID
            u32 iFound = u32MAX;
            newlyCreatedAssetList.forEach( [uidDeleted, &iFound](u32 index, const asset::UID &uid){
                if (uidDeleted == uid)
                {
                    iFound = index;
                    return false;
                }
                return true;
            });

            if (u32MAX == iFound)
            {
                logger->log ("%016" PRIX64 " deleted\n", uidDeleted);
            }
            else
            {
                logger->log ("%016" PRIX64 " rebuilded\n", newlyCreatedAssetList(iFound));
                newlyCreatedAssetList.removeAndSwapWithLast (iFound);
            }
        }

        for (u32 i=0; i<newlyCreatedAssetList.getNElem(); i++)
        {
            logger->log ("%016" PRIX64 " newly builded\n", newlyCreatedAssetList(i));
        }

        logger->decIndent();
    }
    return 0;
}

//***********************************
u32 Builder::priv_collectResInfo (ResList &out_list)
{
    out_list.reset();

    //elenco delle risorse su hard-disk
    priv_collectResourcesFromDisk(out_list);
    priv_collectIniFileFromDisk(out_list);

    //elenco delle risorse storate nel DB
    ResList listDB(localAllocator, 256);
    char s[512];
    db::RST rst;

    sprintf_s (s, sizeof(s), "SELECT UID,lastTimeMod,type,name FROM " GOS_ASSET__TABLE_RES_LIST " ORDER BY UID");
    if (!db::query (ctx.db, s, &rst))
    {
        logger->err ("Builder::priv_collectResInfo() => query error\n");
        return 1;
    }
    while (rst.fetchRow())
    {
        sResListElem elem;
        elem.reset();
        elem.uid._uid = rst.getValAsU64(0);
        elem.lastTimeModified = rst.getValAsU64(1);
        elem.resType = static_cast<eResType>(rst.getValAsU8(2));
        sprintf_s (elem.name, sizeof(elem.name), "%s", rst.getVal(3));

        listDB.append(elem);        
    }


    //ora confronto le 2 liste per determinare quali risorse sono nuove, quali sono eliminate, quali aggiornate
    for (u32 i=0; i<out_list.getNElem(); i++)
    {
        sResListElem *elem = &out_list.getElem(i);

        for (u32 i2=0; i2<listDB.getNElem(); i2++)
        {
            sResListElem *elemDB = &listDB.getElem(i2);

            //se UID esiste gia' nel DB, allora i casi sono 2:
            //  - la risorsa e' rimasta inalterata
            //  - la risorsa e' stata modificata
            if (elem->uid == elemDB->uid)
            {
                if (elem->lastTimeModified > elemDB->lastTimeModified)
                    elem->status = eBuildStatus::MODIFIED;
                else
                    elem->status = eBuildStatus::UNCHANGED;

                //dato che ho trovato una risorsa che matcha con quelle nel DB, ne 
                //aggiorno lo stato anche sulla listDB
                elemDB->status = elem->status;
                break;
            }

            //listDB e' ordinata per UID
            if (elemDB->uid > elem->uid)
                break;
        }

        //se ho trovato uno status, ho finito con questa risorsa
        if (elem->status != eBuildStatus::DONT_KNOW)
            continue;

        //a questo punto la risorsa e' NEW visto che non esisteva nel DB
        elem->status = eBuildStatus::NEW;
    }

    //Tutte le risorse nella listDB che non sono in stato DONT_KNOW vuol dire che sono state DELETED dato che non 
    //le ho trovate nela lista delle res presenti su disco.
    //Le aggiungo alla lista definitiva
    for (u32 i=0; i<listDB.getNElem(); i++)
    {
        if (eBuildStatus::DONT_KNOW == listDB(i).status)
        {
            sResListElem elem = listDB(i);
            elem.status = eBuildStatus::DELETED;
            out_list.append(elem);
        }
    }

    return 0;
}

//***********************************
void Builder::priv_collectResourcesFromDisk (ResList &out_list)
{
    u32 iter;
    asset::eResType resType;
    asset::resType_enumerate_begin (&iter);
    while (asset::resType_enumerate_fetch(iter, &resType))
    {
        char folderName[512];
        asset::res_get_folderNameByType (ctx, resType, folderName, sizeof(folderName));
        priv_collectResourcesFromDisk_ric (folderName, NULL, resType, out_list);
    }
}

//***********************************
void Builder::priv_collectResourcesFromDisk_ric (const char *baseFolderName, const char *subFolder, eResType resType, ResList &out_list)
{
    char fullFolderName[1024];

    if (NULL != subFolder)
        sprintf_s (fullFolderName, sizeof(fullFolderName), "%s/%s", baseFolderName, subFolder);
    else
        sprintf_s (fullFolderName, sizeof(fullFolderName), "%s", baseFolderName);

    logger->log ("%s\n", fullFolderName);
    gos::FileFind ff;
    if (!fs::findFirst (&ff, fullFolderName, "*.*"))
        return;

    bool bAnySubfolder = false;
    do
    {
        const char *fname = fs::findGetFileName(ff);
        
        if (fs::findIsDirectory(ff))
        {
            if (fname[0] != '.')
                bAnySubfolder = true;
            continue;
        }

        sResListElem elem;
        elem.reset();

        if (NULL == subFolder)
            sprintf_s (elem.name, sizeof(elem.name), "%s", fname);
        else
            sprintf_s (elem.name, sizeof(elem.name), "%s/%s", subFolder, fname);
        elem.resType = resType;
        asset::res_createUID (elem.resType, elem.name, &elem.uid);
        elem.lastTimeModified = fs::findGetLastTimeModified_UTC_niceu64 (ff, fullFolderName);
        
        out_list.append(elem);

    } while (fs::findNext(ff));
    
    fs::findClose(ff);


    //processa le subdir
    if (bAnySubfolder)
    {
        if (!fs::findFirst (&ff, fullFolderName, "*.*"))
            return;

        do
        {
            const char *fname = fs::findGetFileName(ff);
            if (fs::findIsDirectory(ff) && fname[0] != '.')
            {
                char s[1024];

                if (NULL != subFolder)
                    sprintf_s (s, sizeof(s), "%s/%s", subFolder, fname);
                else
                    sprintf_s (s, sizeof(s), "%s", fname);                
                priv_collectResourcesFromDisk_ric (baseFolderName, s, resType, out_list);
            }
        } while (fs::findNext(ff));
        
        fs::findClose(ff);
    }
}

//***********************************
void Builder::priv_collectIniFileFromDisk (ResList &out_list)
{
    gos::FileFind ff;
    if (fs::findFirst (&ff, ctx.folder_assets_src, "*.gosasset_d"))
    {
        do
        {
            if (fs::findIsDirectory(ff))
                continue;

            const char *filename = fs::findGetFileName(ff);
            
            //skippo i file che iniziano con __ (dato che li creo/uso come file di appoggio durante il build)
            if (filename[0] == '_')
            {
                if (filename[1] == '_')
                    continue;
            }


            //data ultima modifica
            char s[1024];
            gos::DateTime dt;
            sprintf_s (s, sizeof(s), "%s/%s", ctx.folder_assets_src, filename);
            fs::fileGetLastTimeModified_UTC(s, &dt);

            //aggiungo alla lista di risorse
            sResListElem elem;
            elem.reset();
            sprintf_s (elem.name, sizeof(elem.name), "%s", filename);
            elem.resType = eResType::gosasset_d;
            asset::res_createUID (elem.resType, elem.name, &elem.uid);
            elem.lastTimeModified = dt.getAsNiceU64();            

            out_list.append(elem);

        } while (fs::findNext(ff));
        
        fs::findClose(ff);
    }
}


//***********************************
bool Builder::priv_shaderRes_remove_dependencies (const asset::UID &resUID)
{
    assert (resUID.isAResourceOfType (eResType::shader_txt));

    const u64 MAX_RESOURCE_ID = 0x0000FF0000000000;
    char s[128];
    sprintf_s (s, sizeof(s), "DELETE FROM " GOS_ASSET__TABLE_DEPENDS " WHERE UID=%" PRIu64 " AND childUID < %" PRIu64 "", resUID._uid, MAX_RESOURCE_ID);
    return db::exec (ctx.db, s);
}

//***********************************
u32 Builder::priv_shaderRes_add_dependencies (const ResList &list, u32 me)
{
    assert (list(me).resType == eResType::shader_txt);

    u32 num_errors = 0;
    
    char s[1024];
    asset::res_get_folderNameByType (ctx, eResType::shader_txt, s, sizeof(s));
    strcat_s (s, sizeof(s), "/");
    strcat_s (s, sizeof(s), list(me).name);

    u32 fsize=0;
    u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), s, &fsize);
    if (NULL != buffer)
    {
        string::IncludeDetector det;
        const u32 n = det.parse (buffer, fsize);
        for (u32 i=0; i<n; i++)
        {
            char includeName[128];
            det.getResultAsString (buffer, i, includeName, sizeof(includeName));

            //il nome della risorsa da includere dipende dal path della risorsa base + l'include
            fs::extractFilePathWithOutSlash (list(me).name, s, sizeof(s));
            if (0x00 != s[0])
                strcat_s (s, sizeof(s), "/");
            strcat_s (s, sizeof(s), includeName);
            fs::pathSanitizeInPlace (s);

            //cerco la risorsa nella lista
            bool bFound = false;
            for (u32 i2=0; i2<list.getNElem(); i2++)
            {
                if (list(i2).resType != eResType::shader_txt)
                    continue;
                if (list(i2).status == eBuildStatus::DELETED)
                    continue;

                if (strcmp (list(i2).name, s) == 0)
                {
                    bFound = true;
                    if (!asset::depend_add (ctx, list(me).uid, list(i2).uid))
                    {
                        num_errors++;
                        logger->err ("Error adding dependencies for resource type [shader_txt], '%s', UID=%016" PRIX64 " that depends on '%s', UID%016" PRIX64 "",
                            list(me).name, list(me).uid._uid,
                            list(i2).name, list(i2).uid._uid);
                    }
                    break;
                }
            }

            if (!bFound)
            {
                num_errors++;
                logger->err ("Error adding dependencies for resource type [shader_txt], '%s', UID=%016" PRIX64 "\nCan't find a resource name %s\n", list(me).name, list(me).uid._uid, s);
            }

            //
        }

        GOSFREE_SCRAP (buffer);
    }    

    return num_errors;
    
}


//***********************************
void Builder::priv_explodeIniFile_adjustSubsectionName (const char *subsec_name, char *out, u32 sizeof_out)
{
    assert (subsec_name[0] == '@');

    sprintf_s (out, sizeof_out, "%s", subsec_name);
    u32 len = static_cast<u32>(strlen(out));

    len-=3;
    while (out[len] != '@')
        len--;
    out[len] = 0;

    char num[16];
    sprintf_s (num ,sizeof(num), "@%d@", nextTempSubsectionIndex++);
    strcat_s (out, sizeof_out, num);
}

//***********************************
bool Builder::priv_explodeScript_ric (gos::IniFileSection *dst, gos::IniFileSection *src, const char *nameOfSRC, const asset::UID &uid_of_iniFile)
{
    if (NULL == src)
        return true;

    //tutte le subsection di src le devo trasformare in sezioni di dst
    for (u32 i=0; i<src->getNSubsection(); i++)
    {
        //deve essere di tipo direttiva, altrimenti e' un errore
        gos::IniFileSection *subsecSRC = src->getSubsectionByIndex(i);
        if (subsecSRC->name.getBuffer()[0] != '@')
        {
            logger->err ("invalid subsection, it must start with @\n");
            return false;
        }

        //se la sezione non ha un suo __value, gliene assegno uno d'ufficio
        if (!subsecSRC->exists ("__value"))
        {
            //gli devo assegnare un __value
            char s[32];
            sprintf_s (s, sizeof(s), "__assname_%06d", nextTempNameIndex++);
            subsecSRC->set ("__value", s);
        }

        if (subsecSRC->getNIdentifier() > 1)
            priv_explodeScript_ric (dst, subsecSRC, nameOfSRC, uid_of_iniFile);
    }

    //copio me stesso in dst
    const char *subsec_name = src->name.getBuffer();
    if (NULL != subsec_name)
    {
        char s[256];
        priv_explodeIniFile_adjustSubsectionName (subsec_name, s, sizeof(s));
        
        gos::IniFileSection *subsecDST = dst->addSubsection (s);
        {
            //da quale file gosres_d dove viene questo asset?
            sprintf_s (s, sizeof(s), "%s@%d", nameOfSRC, src->getLineStarted());
            subsecDST->set ("__decl", s);
            subsecDST->setU64 ("__decl_uid", uid_of_iniFile._uid, true);

            eAssetType assType;
            u32 depth = priv_fromSectionNameToAssetDepthAndType (subsec_name, &assType);
            if (u32MAX == depth)
            { 
                
                logger->err ("builder for asset of type %s does not exits\n", asset::enumToString(assType));
                return false; 
            }
            subsecDST->set ("__depth", depth, true);
            subsecDST->set ("__assType", static_cast<u8>(assType), true);

            //copio tutti i parametri della sezione src in subsecDST
            for (u32 i=0; i<src->getNIdentifier(); i++)
            {
                subsecDST->set (src->getIdentifierByIndex(i), src->getValueByIndex(i), true);
            }

            //copio tutte le mie subsection di primo livello
            for (u32 i=0; i<src->getNSubsection(); i++)
            {
                char subsecName[128];
                const gos::IniFileSection *subsecSRC = src->getSubsectionByIndex(i);
                priv_explodeIniFile_adjustSubsectionName (subsecSRC->name.getBuffer(), subsecName, sizeof(subsecName));    
                
                gos::IniFileSection *sub = subsecDST->addSubsection (subsecName);
                {
                    sprintf_s (s, sizeof(s), "%s@%d", nameOfSRC, subsecSRC->getLineStarted());
                    sub->set ("__decl", s);

                    depth = priv_fromSectionNameToAssetDepthAndType (subsecName, &assType);
                    if (u32MAX == depth){ logger->err ("error calculating depth...\n"); return false; }        
                    sub->set ("__depth", depth, true);
                    sub->set ("__assType", static_cast<u8>(assType), true);

                    subsecSRC->get("__value", s, sizeof(s));
                    sub->set ("__value", s);
                }
            }  
        }
    }
    return true;
}



/***********************************
 * Prende il input il file Ini che contiene tutte gli asset in formato "esploso"
 * e comincia a buildarli, uno per uno
 */
u32 Builder::priv_build_explodedIniFileInFolder (gos::IniFile &ini, bool doCreateAssetsFile, FastUIDList &newlyCreatedAssetList)
{
    const u32 numSection = ini.getNSubsection();
    if (0 == numSection)
    {
        logger->log ("nothing to do\n");
        return 0;
    }

    u32 maxDepth = 1;
    for (u32 i=0; i<NUM_MAX_ASSET_BUILDER; i++)
    {
        if (u32MAX == depthByAssetTypeList[i])
            continue;
        if (depthByAssetTypeList[i] > maxDepth)
            maxDepth = depthByAssetTypeList[i];
    }

    //buildo gli asset in ordine di "__depth", dal piu' semplice al piu' complesso
    u32 num_errors = 0;
    for (u32 depth=1; depth<=maxDepth; depth++)
    {
        //troppi errori..termino prematuramente
        if (num_errors > 0)
            break;

        for (u32 secnum=0; secnum<numSection; secnum++)
        {
            IniFileSection *sec = ini.getSubsectionByIndex(secnum);
            if (depth != sec->getOrDefaultAsU32("__depth", u32MAX))
                continue;

            //segno che questa sezione l'ho processata cosi' al prossimo giro la skippo
            sec->set ("__depth", u32MAX, false);

            //recupero asset-type
            eAssetType assType = static_cast<eAssetType> (sec->getOrDefaultAsU8 ("__assType", 0));

            //recupero runtimeName
            char runtimeName[128];
            runtimeName[0] = 0x00;
            sec->get ("__value", runtimeName, sizeof(runtimeName));

            //recupero il nome del file .gosres_d in cui questo asset viene dichiarato
            char fromSRC[256];
            fromSRC[0] = 0x00;
            sec->get ("__decl", fromSRC, sizeof(fromSRC));


            //info a video
            logger->log (eTextColor::grey, "parsing section '%s: %s from %s'\n", sec->name.getBuffer(), runtimeName, fromSRC);

            //cerco un builder appropriato
            BuilderInterface *builder = priv_getBuilder(assType);
            if (NULL == builder)
            {
                num_errors++;
                logger->err ("can't find a Builder to build this asset.\n");
            }
            else
            {
                //buildo
                logger->incIndent();

                asset::UID uid_of_iniFile;
                uid_of_iniFile._uid = sec->getOrDefaultAsU64("__decl_uid", 0);
                num_errors += priv_build_iniSection (doCreateAssetsFile, sec, uid_of_iniFile, fromSRC, builder, runtimeName, newlyCreatedAssetList);
                logger->decIndent();
            }
        }
    }

    return num_errors;
}

//***********************************
u32 Builder::priv_build_iniSection (bool doCreateAssetsFile, const IniFileSection *sec, const asset::UID &uid_of_iniFile, const char *sourceFileInfo, BuilderInterface *builder, const char *runtimeName, FastUIDList &newlyCreatedAssetList)
{
    asset::sBuildResult result;
    if (!builder->build (ctx, buildTimeUTC, sourceFileInfo, uid_of_iniFile, sec, doCreateAssetsFile, &result))
        return 1;
    assert (result.uid.getAssetDepth() == priv_getDepthByAssetType (result.uid.getAssetType()));

    //report a video del risultato della build
    eTextColor color = eTextColor::green;
    if (eBuildResult::was_already_built == result.result)
        color = eTextColor::darkBlue;
    logger->log (color, "%016" PRIX64 " [%-17s] \n", result.uid._uid, asset::enumToString(result.result));


    //calcolo e scrivo le dipendenze runtime di questo asset
    //Per "dipendenze runtime" intendo una lista di altri asset (e non risorse) dai quali questo asset dipende
    if (eBuildResult::just_built == result.result)
    {
        newlyCreatedAssetList.append (result.uid);

        if (result.uid.getAssetDepth() > 1)
        {
            HashedUIDList   hashList1 (localAllocator, 256);
            asset::asset_get_dependecies_list (ctx, result.uid, true, &hashList1);

            auto list = hashList1._queryList();
            for (u32 i=0; i<list->getNElem(); i++)
            {
                asset::UID childUID;
                childUID = list->queryElem(i).key;
                if (childUID.isAnAsset())
                {
                    asset::dependRT_add (ctx, result.uid, childUID, priv_getDepthByAssetType (childUID.getAssetType()));
                }
            }
        }    
    }


    /*  ok, l'asset e' stato buildato con successo ed e' stato inserito nel DB
        Se e' associato ad un runtimeName, devo registrare il link
    */
    if (NULL == runtimeName)
        return 0;


    asset::UID uid;
    color = eTextColor::darkBlue;            
    if (!asset::rtname_exists (ctx, runtimeName, &uid))
    {
        if (!asset::rtname_insert (ctx, runtimeName, result.uid))
            return 1;

        if (runtimeName[0] != '_' && runtimeName[1] != '_')
        {
            color = eTextColor::green;

            //dato che l'asset ha un nuovo runtime name, devo aggiungere la dipendenza dal file gosaaset_d dove il runtime name
            //e' stato dichiarato
            if (!asset::depend_exists(ctx, result.uid, uid_of_iniFile))
                asset::depend_add (ctx, result.uid, uid_of_iniFile);
        }
        logger->log (color, "%016" PRIX64 " is now known as '%s'\n", result.uid._uid, runtimeName);
        return 0;
    }

    //il runtimeName esiste gia' nel DB.
    //Se punta allo stesso UID, tutto bene, altrimenti c'e' un problema
    if (uid == result.uid)
    {
        logger->log (color, "%016" PRIX64 " is now known as '%s'\n", result.uid._uid, runtimeName);
        return 0;
    }


    logger->err ("runtimeName '%s' is already linked to UID=%016" PRIX64 " (use another name)\n", runtimeName, uid._uid);
    return 1;
}


