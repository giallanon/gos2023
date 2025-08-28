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
    }
}

//***********************************
Builder::Builder()
{
    localAllocator = gos::getSysHeapAllocator();
    builderList.setup (localAllocator, 32);

    addBuilder<Builder_vtxShader>();
    addBuilder<Builder_pxlShader>();
    addBuilder<Builder_pipeDef>();
}

//***********************************
Builder::~Builder()
{
    for (u32 i=0; i<builderList.getNElem(); i++)
    {
        BuilderInterface *builder = builderList[i];
        GOSDELETE(localAllocator, builder);
    }

    builderList.unsetup();
    
    asset::context_close (ctx);
}

//***********************************
bool Builder::priv_addBuilder (BuilderInterface *builder)
{
    assert (NULL != builder);
    for (u32 i=0; i<builderList.getNElem(); i++)
    {
        if (builderList(i)->getAssType() == builder->getAssType())
        {
            logger::err ("asset::Builder::priv_addBuilder() => a builder for res %s already exists\n", asset::enumToString(builder->getAssType()));
            return false;
        }
    }

    builderList.append (builder);
    return true;
}

//***********************************
BuilderInterface* Builder::priv_getBuilder (eAssetType assType)
{
    for (u32 i=0; i<builderList.getNElem(); i++)
    {
        if (builderList(i)->getAssType() == assType)
        {
            return builderList[i];
        }
    }
    
    return NULL;    
}

//***********************************
void Builder::priv_printResList (const ResList &list) const
{
    for (u32 i=0; i<list.getNElem(); i++)
    {
        gos::DateTime dt;
        dt.setFromNiceU64 (list(i).lastTimeModified);

        char lastTimeMod[64];
        dt.formatAs_YYYYMMDDHHMMSS (lastTimeMod, sizeof(lastTimeMod));


        logger::log ("%-10s %016" PRIX64 " %-64s [%-12s] % 20s\n", 
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
u32 Builder::priv_fromSectionNameToAssetDeepAndType (const char *name, eAssetType *out_assType) const
{
    /*  deep rappresenta il numero massimo di subsection annidate all'interno di una secion di tipo asset.
        Maggiore e' deep, piu' tardi viene builadata la risorsa.
        L'idea e' di buildare prima gli asset piu' semplice, per es gli shader che di fatto non dipendono
        da nessun altro asset.
        A seguire si buildano quelli che dipendono da un solo asset e via dicendo.
        Deep pero' non rappresenta il num di asset da cui dipende una risorsa, ma rappresenta la profondita'
        dell'alvero che descrive la rirosa.
        Es:
            @section  (deep=0)
            {
                param
                ..
                param
            }

            @section  (deep=1)
            {
                param
                ..
                param

                @sebsection
                {
                    param...
                }
            }            

            @section  (deep=2)
            {
                param
                ..
                param

                @sebsection
                {
                    param...

                    @sebsection
                    {
                    }
                }
            }            
    */

    if (!priv_fromSectionNameToAssetType(name, out_assType))
        return u32MAX;

    switch (*out_assType)
    {
    default:    
        DBGBREAK;
        return u32MAX;
    
    case eAssetType::vtx_shader:    return 0;
    case eAssetType::pxl_shader:    return 0;
    case eAssetType::pipeline_def:  return 1;
    }
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

        logger::log (eTextColor::blue, "scanning resource folders...\n");
        logger::incIndent();
        {
            ResList tempList(localAllocator, 256);
            num_errors += priv_collectResInfo(tempList);
            logger::incIndent();
                priv_printResList (tempList);
            logger::decIndent();

            //elimino le risorse UNCHANGED
            for (u32 i=0; i<tempList.getNElem(); i++)
            {   
                if (eBuildStatus::UNCHANGED != tempList(i).status)
                    updatedResList.append (tempList(i));
            }
        }
        logger::decIndent();



        /*esplosione dei fine gosres_d
            Prendo tutti i file gosres_d in lista e li esplodo il che vuol dire che creo un nuovo IniFile che contiene tutti gli asset da creare.
            Per ogni asset che ha un sotto-asset, esplodo il file ini tirando fuori la sottosezione
            Alla fine della procudera, ho un IniFile in cui anche gli asset con sottorisorsa sono sempre espressi come asset che si riferiscono ad un runtimeName
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
        priv_explodeIniFile (updatedResList, &iniExploded);



        //in ordine ora si devono processare prima le risorse deleted, poi le update e per ultimo le new
        //...
        //...

        //infine processo tutti gli IniFile che sono in stato di NEW
        logger::log (eTextColor::blue, "building NEW assets...\n");
        logger::incIndent();
            num_errors += priv_build_explodedIniFileInFolder (iniExploded);
        logger::decIndent();


    logger::decIndent();

    //se tutto ok, ho finito
    char s[1024];
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
    priv_collectIniFileFromFDisk(out_list);

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


    //just for fun, inserisco tutte le NEW
    for (u32 i=0; i<out_list.getNElem(); i++)
    {
        if (eBuildStatus::NEW == out_list(i).status)
            asset::res_insert (ctx, out_list(i).uid, out_list(i).lastTimeModified, out_list(i).resType, out_list(i).name);
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
void Builder::priv_collectIniFileFromFDisk (ResList &out_list)
{
    gos::FileFind ff;
    if (fs::findFirst (&ff, ctx.folder_assets_src, "*.gosasset_d"))
    {
        do
        {
            if (fs::findIsDirectory(ff))
                continue;

            char s[1024];

            //data ultima modifica
            gos::DateTime dt;
            sprintf_s (s, sizeof(s), "%s/%s", ctx.folder_assets_src, fs::findGetFileName(ff));
            fs::fileGetLastTimeModified_UTC(s, &dt);

            //aggiungo alla lista di risorse
            sResListElem elem;
            elem.reset();
            sprintf_s (elem.name, sizeof(elem.name), "%s", fs::findGetFileName(ff));
            elem.resType = eResType::iniFile;
            asset::res_createUID (elem.resType, elem.name, &elem.uid);
            elem.lastTimeModified = dt.getAsNiceU64();            

            out_list.append(elem);

        } while (fs::findNext(ff));
        
        fs::findClose(ff);
    }
}



//***********************************
u32 Builder::priv_explodeIniFile (ResList &list, gos::IniFile *out)
{
    u32 num_errors = 0;

    char s[1024];
    sprintf_s (s, sizeof(s), "%s/all_assets.txt", ctx.folder_assets_src);
    out->createEmpty (s);


    for (u32 i=0; i<list.getNElem(); i++)
    {
        if (eResType::iniFile != list(i).resType)
            continue;

        if (eBuildStatus::NEW == list(i).status || eBuildStatus::MODIFIED == list(i).status)
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
            if (!priv_explodeIniFile_ric (out->getRoot(), ini.getRoot(), list(i).name))
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
bool Builder::priv_explodeIniFile_ric (gos::IniFileSection *dst, gos::IniFileSection *src, const char *nameOfSRC)
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
            priv_explodeIniFile_ric (dst, subsecSRC, nameOfSRC);
    }

    //copio me stesso in dst
    const char *subsec_name = src->name.getBuffer();
    if (NULL != subsec_name)
    {
        char s[256];
        priv_explodeIniFile_adjustSubsectionName (subsec_name, s, sizeof(s));
        
        gos::IniFileSection *subsecDST = dst->addSubsection (s);
        {
            subsecDST->set ("__decl", nameOfSRC);

            eAssetType assType;
            u32 deep = priv_fromSectionNameToAssetDeepAndType (subsec_name, &assType);
            if (u32MAX == deep) { logger::err ("error calculating deep...\n"); return false; }
            subsecDST->set ("__deep", deep, true);
            subsecDST->set ("__assType", static_cast<u8>(assType), true);

            //copio tutti i parametri della sezione src in subsecDST
            for (u32 i=0; i<src->getNIdentifier(); i++)
            {
                subsecDST->set (src->getIdentifierByIndex(i), src->getValueByIndex(i), true);
            }

            //copio tutte le mie subsection di primo livello
            for (u32 i=0; i<src->getNSubsection(); i++)
            {
                const gos::IniFileSection *subsecSRC = src->getSubsectionByIndex(i);
                priv_explodeIniFile_adjustSubsectionName (subsecSRC->name.getBuffer(), s, sizeof(s));

                gos::IniFileSection *sub = subsecDST->addSubsection (s);
                {
                    sub->set ("__decl", nameOfSRC);

                    deep = priv_fromSectionNameToAssetDeepAndType (s, &assType);
                    if (u32MAX == deep){ logger::err ("error calculating deep...\n"); return false; }        
                    sub->set ("__deep", deep, true);
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

    //buildo gli asset in ordine di "__deep"
    u32 num_errors = 0;
    u32 deep = 0;
    bool bEsci = false;
    while (bEsci == false)
    {
        bEsci = true;
        for (u32 secnum=0; secnum<numSection; secnum++)
        {
            IniFileSection *sec = ini.getSubsectionByIndex(secnum);

            if (deep != sec->getOrDefaultAsU32("__deep", u32MAX))
                continue;

            //segno che questa sezione l'ho processata
            bEsci = false;
            sec->set ("__deep", u32MAX, false);

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



            logger::log (eTextColor::grey, "parsing section '%s: %s from %s'\n", sec->name.getBuffer(), runtimeName, fromSRC);

            //cerco un builder appropriato
            BuilderInterface *builder = priv_getBuilder(assType);
            if (NULL == builder)
            {
                num_errors++;
                logger::err ("can't find a Builder to build this asset.\n");
                continue;
            }

            logger::incIndent();
            num_errors += priv_build_iniSection (sec, builder, runtimeName);
            logger::decIndent();
        }


        deep++;
    }

    return num_errors;
}

//***********************************
u32 Builder::priv_build_iniSection (const IniFileSection *sec, BuilderInterface *builder, const char *runtimeName)
{
    u32 num_errors = 0;

    asset::sBuildResult result;
    if (!builder->build (ctx, buildTimeUTC, sec, &result))
    {
        num_errors++;
        return num_errors;
    }

    //ok, l'asset e' stato buildato con successo ed e' stato inserito nel DB
    //Se e' associato ad un runtimeName..
    /*
    TODO:   bisogna prima accertarsi che i runtimeName della roba da buildare non siano
            duplicati. Ci pensiamo dopo aver risolto la questione delle dipendenze tra risorse!
    if (NULL != runtimeName)
    {
        asset::UID oldUID;
        if (!asset::rtname_insert_or_update (ctx, runtimeName, result.uid, &oldUID))
        {
            num_errors++;
            return num_errors;
        }
        
        if (oldUID.isValid())
        {
            //il <runtimeName> era associato ad un assetUID il che vuol dire che tutti gli asset che
            //facevano riferimento a quello assetUID adesso vanno rebuildati
        }
    }
    */
    
    //report finale a video
    eTextColor color = eTextColor::green;
    if (eBuildResult::was_already_built == result.result)
        color = eTextColor::darkBlue;
    logger::log (color, "[%-17s] %016" PRIX64 "\n", asset::enumToString(result.result), result.uid._uid);
    
    return num_errors;
}
