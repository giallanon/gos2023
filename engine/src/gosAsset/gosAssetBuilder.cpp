#include "gosAssetBuilder.h"
#include "gos.h"
#include "builders/gosAssetBuilder_shader.h"
#include "builders/gosAssetBuilder_pipedef.h"

using namespace gos;
using namespace gos::asset;


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
    case eBuildStatus::REQUIRED:    return "required";
    }
}

//***********************************
Builder::Builder()
{
    localAllocator = gos::getSysHeapAllocator();
    
    
    //suppongo un massimo di NUM_MAX_ASSET_BUILDER tipo di asset diversi
    u32 size;
    size = sizeof(BuilderInterface*) * NUM_MAX_ASSET_BUILDER;
    builderList = GOSALLOCT(BuilderInterface**, localAllocator, size);
    memset (builderList, 0, size);

    size = sizeof(u32) * NUM_MAX_ASSET_BUILDER;
    depthByAssetType = GOSALLOCT(u32*, localAllocator, size);
    memset (depthByAssetType, 0xFF, sizeof(u32) * NUM_MAX_ASSET_BUILDER);


    addBuilder<Builder_vtxShader>();
    addBuilder<Builder_pxlShader>();
    addBuilder<Builder_pipeDef>();
}

//***********************************
Builder::~Builder()
{
    for (u32 i=0; i<NUM_MAX_ASSET_BUILDER; i++)
    {
        if (NULL == builderList[i])
            continue;
        GOSDELETE(localAllocator, builderList[i]);
    }

    GOSFREE(localAllocator, builderList);
    builderList = NULL;
    
    GOSFREE(localAllocator, depthByAssetType);
    depthByAssetType = NULL;
    
    asset::context_close (ctx);
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
        depthByAssetType[index] = asset_depth;
        return true;
    }
    
    logger::err ("asset::Builder::priv_addBuilder() => a builder for res %s already exists\n", asset::enumToString(builder->getAssType()));
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

        case eBuildStatus::REQUIRED:
            color = eTextColor::grey;
            break;

        case eBuildStatus::DELETED:
            color = eTextColor::red;
            break;
        }

        gos::DateTime dt;
        dt.setFromNiceU64 (list(i).lastTimeModified);

        char lastTimeMod[64];
        dt.formatAs_YYYYMMDDHHMMSS (lastTimeMod, sizeof(lastTimeMod));            

        logger::log (color, "%-10s %016" PRIX64 " %-64s [%-12s] % 20s\n", 
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
bool Builder::rebuildAll(const char *baseFolder)
{
    asset::context_close (ctx);

    bool ret = true;
    logger::log (eTextColor::white, "asset::Builder::rebuildAll()\n");
    logger::incIndent();
    {
        //elimina il DB
        char s[512];
        sprintf_s (s, sizeof(s), "%s/" GOS_ASSET__DB_NAME "", baseFolder);
        fs::fileDelete(s);

        //elimina tutti gli asset mai creati
        asset::asset_get_binfolder_name (baseFolder, s, sizeof(s));
        fs::folderDeleteAllFileWithJolly (s, "*.gosasset");

        //builda
        ret = buildAll(baseFolder);
        
    }
    logger::decIndent();

    return ret;
}

//***********************************
bool Builder::buildAll (const char *baseFolder)
{
    asset::context_close (ctx);
    
    if (!asset::context_open (baseFolder, &ctx))
    {
        logger::err ("asset::Builder::rebuildAll() => invalid ctx\n");
        return false;
    }

    //data e ora di build
    char s[1024];
    gos::DateTime dt;
    dt.setNow_UTC();
    this->buildTimeUTC = dt.getAsNiceU64();
    nextTempNameIndex = nextTempSubsectionIndex = 0;

    //faccio un backup del DB in caso qualcosa vada male
    asset::context_cloneDB (ctx, ".backup");


    u32 num_errors = 0;
    logger::log (eTextColor::white, "asset::Builder::buildAll()\n");
    logger::incIndent();
        
        //elenco delle risorse e dei gosres_d che sono stati modificati, cancellati o creati nuovi
        ResList updatedResList(localAllocator, 256);
        {
            logger::log (eTextColor::blue, "scanning resource folders...\n");
            logger::incIndent();
            {
                ResList tempList(localAllocator, 256);
                num_errors += priv_collectResInfo (tempList);

                //Elimino le risorse UNCHANGED dalla lista.
                //Aggiorno il DB lastTimeMod delle risorse MODIFIED
                //Inserisco le NEW nel DB
                for (u32 i=0; i<tempList.getNElem(); i++)
                {   
                    if (eBuildStatus::UNCHANGED != tempList(i).status)
                        updatedResList.append (tempList(i));

                    switch (tempList(i).status)
                    {
                    default:
                    case eBuildStatus::DONT_KNOW:
                        //questo non deve mai succede
                        DBGBREAK;
                        break;

                    case eBuildStatus::UNCHANGED:
                        break;

                    case eBuildStatus::NEW:
                        if (!asset::res_insert (ctx, tempList(i).uid, tempList(i).lastTimeModified, tempList(i).resType, tempList(i).name))
                            num_errors++;
                        break;

                    case eBuildStatus::MODIFIED:
                        if (!asset::res_update (ctx, tempList(i).uid, buildTimeUTC))
                            num_errors++;
                        break;

                    case eBuildStatus::DELETED:
                        //TODO
                        num_errors++;
                        DBGBREAK;
                        break;
                    }
                }

                logger::incIndent();
                    priv_printResList (updatedResList);
                logger::decIndent();

            }
            logger::decIndent();
        }

    //Ho collezionato le risorse che sono state modificate, cancellate o create nuove.
    //Controllo le risorse "modificate" e per ciascuna di queste verifico quali asset la richiedono.
    //Gli asset che richiedono una risorsa MODIFIED, devono a loro volta essere rebuildate
    if (0 == num_errors)
    {
        if (!priv_explode_NEW_or_MODIFIED_res (&updatedResList))
            num_errors++;
        else
        {
            logger::log (eTextColor::blue, "including dependencies...\n");
            logger::incIndent();
                priv_printResList (updatedResList);
            logger::decIndent();

        }        
    }


    if (0 == num_errors)
    {
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
        iniExploded.setup (localAllocator);
        num_errors += priv_explodeIniFile (updatedResList, &iniExploded);
            
        if (0 == num_errors)
        {
            //in ordine ora si devono processare prima le risorse deleted, poi le update e per ultimo le new
            //...
            //...

            //infine processo tutti gli IniFile che sono in stato di NEW
            logger::log (eTextColor::blue, "building NEW assets...\n");
            logger::incIndent();
                num_errors += priv_build_explodedIniFileInFolder (iniExploded);
                if (0 == num_errors)
                {
                    //clean up del DB
                    //Elimino tutti i runtimeName che iniziano con __ dato che li ho creati io artificialmente durante il build
                    db::exec (ctx.db, "DELETE FROM " GOS_ASSET__TABLE_RUNTIME_NAME " WHERE name LIKE '!_!_%' escape '!'");
                }
            logger::decIndent();
        }
    }

    //finito
    logger::decIndent();

    //se tutto ok, ho finito
    if (0 == num_errors)
    {
        asset::context_close (ctx);
        sprintf_s (s, sizeof(s), "%s/" GOS_ASSET__DB_NAME ".backup", baseFolder);
        fs::fileDelete(s);
        return true;
    }

    logger::err ("There were errors (%d)\n", num_errors);
    logger::log ("Restoring previous db\n");
    
    //ripristino il vecchio DB
    asset::context_cloneDB (ctx, ".failed");
    asset::context_close (ctx);
    
    
    sprintf_s (s, sizeof(s), "%s/" GOS_ASSET__DB_NAME "", baseFolder);
    fs::fileDelete(s);

    fs::fileRename (baseFolder, GOS_ASSET__DB_NAME ".backup", GOS_ASSET__DB_NAME);
    return false;
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
        gos::logger::err ("Builder::priv_collectResInfo() => query error\n");
        return 1;
    }
    while (rst.fetchRow())
    {
        sResListElem elem;
        elem.reset();
        elem.uid._uid = rst.getColValueAsU64(0);
        elem.lastTimeModified = rst.getColValueAsU64(1);
        elem.resType = static_cast<eResType>(rst.getColValueAsU8(2));
        sprintf_s (elem.name, sizeof(elem.name), "%s", rst.getColValue(3));

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
    asset::res_enumerate_begin (&iter);
    while (asset::res_enumerate_fetch(iter, &resType))
    {
        char folderName[512];
        asset::res_get_folder_name (ctx, resType, folderName, sizeof(folderName));

        logger::log ("%s\n", folderName);
        logger::incIndent();
        {
            gos::FileFind ff;
            if (fs::findFirst (&ff, folderName, "*.*"))
            {
                do
                {
                    if (fs::findIsDirectory(ff))
                        continue;

                    char s[1024];
                    gos::DateTime dt;
                    sprintf_s (s, sizeof(s), "%s/%s", folderName, fs::findGetFileName(ff));
                    fs::fileGetLastTimeModified_UTC(s, &dt);

                    sResListElem elem;
                    elem.reset();
                    sprintf_s (elem.name, sizeof(elem.name), "%s", fs::findGetFileName(ff));
                    elem.resType = resType;
                    asset::res_createUID (elem.resType, elem.name, &elem.uid);
                    elem.lastTimeModified = dt.getAsNiceU64();
                    

                    out_list.append(elem);

                } while (fs::findNext(ff));
                
                fs::findClose(ff);
            }
        }
        logger::decIndent();
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
bool Builder::priv_explode_NEW_or_MODIFIED_res (ResList *in_out_list)
{
    asset::HashedUIDList hashList;
    hashList.setup (localAllocator, 1024);

    asset::HashedUIDList requiredByList;
    requiredByList.setup (localAllocator, 1024);

    asset::HashedUIDList dependOnList;
    dependOnList.setup (localAllocator, 1024);

    ResList list;
    list.setup (localAllocator, 1024);

    const u32 n = in_out_list->getNElem();
    for (u32 i=0; i<n; i++)
    {
        if (eBuildStatus::NEW == in_out_list->queryElem(i).status)
        {
            //le risorse NEW finisco certamente nella lista
            list.append ( in_out_list->queryElem(i) );
            continue;
        }

        if (eBuildStatus::MODIFIED == in_out_list->queryElem(i).status)
        {
            asset::UID uid;
            uid = in_out_list->queryElem(i).uid;

            if (!hashList.insertIfNotExists(uid, 0))
                continue;

            //ho trovato una risorsa MODIFIED che non ho ancora preso in considerazione
            //Devo recuperre tutti quelli che dipendono da questa risorsa e aggiungerli alla lista.
            //Attenzione che la lista e' solo una lista di risorse, non deve includere anche gli asset
            list.append ( in_out_list->queryElem(i) );

            if (!asset::res_get_requireBy_list (ctx, uid, true, &requiredByList))
                return false;
            else
            {
                //ho una lista di risorse e asset che dipendono da me.
                //Aggiungo tutte le risorse che dipendono da me.
                //Aggiungo anche tutte le risorse che sono richieste dagli asset che dipendono da me
                auto reqList = requiredByList._queryList();
                const u32 n2 = reqList->getNElem();
                for (u32 i2=0; i2<n2; i2++)
                {
                    asset::UID uid2;
                    uid2 = reqList->queryElem(i2).key;

                    //se sta cosa l'ho gia' processata, skippo
                    if (!hashList.insertIfNotExists(uid2, 1))
                        continue;

                    sResListElem elem;
                    if (uid2.isAResource())
                    {
                        elem.reset();
                        elem.uid = uid2;
                        elem.status = eBuildStatus::REQUIRED;
                        asset::res_get_info (ctx, elem.uid, elem.name, sizeof(elem.name), &elem.resType, &elem.lastTimeModified);

                        list.append (elem);
                    }
                    else
                    {
                        //sono un asset... devo recuperare la lista delle risorse che mi sono necessarie
                        if (!asset::asset_get_dependecies_list (ctx, uid2, true, &dependOnList))
                            return false;
                     
                        auto depList = dependOnList._queryList();
                        const u32 n3 = depList->getNElem();
                        for (u32 i3=0; i3<n3; i3++)
                        {
                            asset::UID uid3;
                            uid3 = depList->queryElem(i3).key;

                            if (uid3.isAResource())
                            {
                                if (!hashList.insertIfNotExists(uid3, 0))
                                    continue;
                                
                                elem.reset();
                                elem.uid = uid3;
                                elem.status = eBuildStatus::REQUIRED;
                                asset::res_get_info (ctx, elem.uid, elem.name, sizeof(elem.name), &elem.resType, &elem.lastTimeModified);
                                list.append (elem);
                            }                            
                        }                            
                    }
                }
            }
        }
    }

    //Se tutto ok, aggiorno la lista <in_out_list> con tutte le nuove risorse da considerare
    in_out_list->reset();
    in_out_list->copyFrom (list);
    return true;
}



//***********************************
u32 Builder::priv_explodeIniFile (ResList &list, gos::IniFile *out)
{
    u32 num_errors = 0;

    char s[1024];
    sprintf_s (s, sizeof(s), "%s/__build.gosasset_d", ctx.folder_assets_src);
    out->createEmpty (s);


    for (u32 i=0; i<list.getNElem(); i++)
    {
        if (eResType::gosasset_d != list(i).resType)
            continue;

        if (eBuildStatus::NEW == list(i).status || eBuildStatus::MODIFIED == list(i).status || eBuildStatus::REQUIRED == list(i).status)
        {
            //Ho un IniFile che e' in stato NEW o MODIFIED, lo devo esplodere
            
            gos::IniFile ini;
            sprintf_s (s, sizeof(s), "%s/%s", ctx.folder_assets_src, list(i).name);
            if (!ini.loadAndParse (s))
            {
                num_errors++;
                return num_errors;
            }

            //scanno tutte le sezioni
            if (!priv_explodeIniFile_ric (out->getRoot(), ini.getRoot(), list(i).name, list(i).uid))
            {
                num_errors++;
                return num_errors;
            }
        }
    }

    out->save();

    return num_errors;
}

//***********************************
void Builder::priv_explodeIniFile_adjustSubsectionName (const char *subsec_name, char *out, u32 sizeof_out)
{
    assert (subsec_name[0] == '@');

    sprintf_s (out, sizeof_out, "%s", subsec_name);
    u32 len = strlen(out);

    len-=3;
    while (out[len] != '@')
        len--;
    out[len] = 0;

    char num[16];
    sprintf_s (num ,sizeof(num), "@%d@", nextTempSubsectionIndex++);
    strcat_s (out, sizeof_out, num);
}

//***********************************
bool Builder::priv_explodeIniFile_ric (gos::IniFileSection *dst, gos::IniFileSection *src, const char *nameOfSRC, const asset::UID &uid_of_iniFile)
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
            logger::err ("invalid subsection, it must start with @\n");
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
            priv_explodeIniFile_ric (dst, subsecSRC, nameOfSRC, uid_of_iniFile);
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
            if (u32MAX == depth) { logger::err ("error calculating depth...\n"); return false; }
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
                    if (u32MAX == depth){ logger::err ("error calculating depth...\n"); return false; }        
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
u32 Builder::priv_build_explodedIniFileInFolder (gos::IniFile &ini)
{
    const u32 numSection = ini.getNSubsection();
    if (0 == numSection)
    {
        logger::log ("nothing to be done here\n");
        return 0;
    }

    //buildo gli asset in ordine di "__depth", dal piu' semplice al piu' complesso
    u32 num_errors = 0;
    u32 depth = 0;
    bool bEsci = false;
    while (bEsci == false)
    {
        bEsci = true;
        for (u32 secnum=0; secnum<numSection; secnum++)
        {
            IniFileSection *sec = ini.getSubsectionByIndex(secnum);
            if (depth != sec->getOrDefaultAsU32("__depth", u32MAX))
                continue;

            //segno che questa sezione l'ho processata cosi' al prossimo giro la skippo
            bEsci = false;
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
            logger::log (eTextColor::grey, "parsing section '%s: %s from %s'\n", sec->name.getBuffer(), runtimeName, fromSRC);

            //cerco un builder appropriato
            BuilderInterface *builder = priv_getBuilder(assType);
            if (NULL == builder)
            {
                num_errors++;
                logger::err ("can't find a Builder to build this asset.\n");
            }
            else
            {
                //buildo
                logger::incIndent();

                asset::UID uid_of_iniFile;
                uid_of_iniFile._uid = sec->getOrDefaultAsU64("__decl_uid", 0);
                num_errors += priv_build_iniSection (sec, uid_of_iniFile, builder, runtimeName);
                logger::decIndent();
            }

            //troppi errori..termino prematuramente
            if (num_errors > 5)
            {
                bEsci = true;
                break;
            }
        }

        depth++;
    }

    return num_errors;
}

//***********************************
u32 Builder::priv_build_iniSection (const IniFileSection *sec, const asset::UID &uid_of_iniFile, BuilderInterface *builder, const char *runtimeName)
{
    asset::sBuildResult result;
    if (!builder->build (ctx, buildTimeUTC, uid_of_iniFile, sec, &result))
        return 1;

    //report a video del risultato della build
    eTextColor color = eTextColor::green;
    if (eBuildResult::was_already_built == result.result)
        color = eTextColor::darkBlue;
    logger::log (color, "%016" PRIX64 " [%-17s] \n", result.uid._uid, asset::enumToString(result.result));


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
            color = eTextColor::green;
        logger::log (color, "%016" PRIX64 " is now known as '%s'\n", result.uid._uid, runtimeName);
        return 0;
    }

    //il runtimeName esiste gia' nel DB.
    //Se punta allo stesso UID, tutto bene, altrimenti c'e' un problema
    if (uid == result.uid)
    {
        logger::log (color, "%016" PRIX64 " is now known as '%s'\n", result.uid._uid, runtimeName);
        return 0;
    }


    logger::err ("runtimeName '%s' is already linked to UID=%016" PRIX64 " (use another name)\n", runtimeName, uid._uid);
    return 1;
}
