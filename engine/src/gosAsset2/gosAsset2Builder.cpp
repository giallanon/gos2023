#include "gosAsset2Builder.h"
#include "gosAsset2.h"
#include "gos.h"
#include "string/gosStringIncludeDetector.h"
#include "builders/gosAsset2Builder_shader.h"
#include "builders/gosAsset2Builder_pipe.h"

using namespace gos;
using namespace gos::asset2;






//****************************** 
Builder::Builder(gos::GPU *gpuIN)
{
	gpu = gpuIN;
	localAllocator = gos::getSysHeapAllocator();
	logger = gos::logger::getSystemLogger();

	memset (builderList, 0, sizeof(builderList));
	addBuilder<Builder_vtxShader>();
	addBuilder<Builder_pxlShader>();
	addBuilder<Builder_pipe>();
}

//****************************** 
Builder::~Builder()
{
	for (u32 i=0; i<NUM_MAX_BUILDERS; i++)
	{
		if (NULL != builderList[i])
		{
			builderList[i]->deinitOnce();
			GOSDELETE(localAllocator, builderList[i]);
		}
	}
}

//***********************************
bool Builder::priv_addBuilder (BuilderInterface *builder)
{
    assert (NULL != builder);
    
    const u32 index = static_cast<u8>(builder->getAssetType());
    assert (index < NUM_MAX_BUILDERS);

    if (NULL == builderList[index])
    {
        builderList[index] = builder;
        builder->initOnce(gpu);
        return true;
    }
    
    logger->err ("Builder::priv_addBuilder() => a builder for res %s already exists\n", asset2::enumToString(builder->getAssetType()));
    return false;
}

//***********************************
const char* Builder::enumToString (eBuildStatus s) const
{
	switch (s)
	{
		default:	DBGBREAK; return "!!ERR::eBuildStatus!!";
		case eBuildStatus::NEW:			return "NEW";
		case eBuildStatus::MODIFIED:	return "MODIFIED";
		case eBuildStatus::DELETED:		return "DELETED";
		case eBuildStatus::UNCHANGED:	return "UNCHANGED";
	}
}

//***********************************
void Builder::priv_printResListElem (const sResListElem &elem) const
{
	eTextColor color = eTextColor::grey;
	switch (elem.status)
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
		//continue;
		break;
	}

	gos::DateTime dt;
	dt.setFromNiceU64 (elem.lastTimeModified);

	char lastTimeMod[64];
	dt.formatAs_YYYYMMDDHHMMSS (lastTimeMod, sizeof(lastTimeMod));            

	logger->log (color, "%-10s [%-12s] %016" PRIX64 " % 20s %s\n", 
		enumToString(elem.status),
		asset2::enumToString (elem.uid.getResourceType()), 
		elem.uid._uid, 
		lastTimeMod,
		elem.abspath
		);	
}

//***********************************
void Builder::priv_printResList (const ResList &list) const
{
    for (u32 i=0; i<list.getNElem(); i++)
    {
		priv_printResListElem(list(i));
    }
}

//***********************************
bool Builder::debug_sanityCheck (const char *baseFolder)
{
	static const char DB_NAME[] = {"sanitycheck.sqlite3"};

    logger = gos::logger::getSystemLogger();
    logger->log (eTextColor::yellow, "\n\n=== RUNNING SANITY CHECK....\n");
    logger->incIndent();

	//faccio un rebuild all usando un nome db specifico
	char s[1024];
	sprintf_s (s, sizeof(s), "%s/%s", baseFolder, DB_NAME);
	fs::fileDelete(s);

	DBContext ctxSanity;
	if (!asset2::dbcontext_open_ex (baseFolder, DB_NAME, &ctxSanity))
	{
		logger->err ("Can't create DB\n");
		return false;
	}
	
	
	bool ret = priv_build (ctxSanity, false);
    if (!ret)
	{
        logger->log (eTextColor::red, "sanity build FAILED\n");
	}
	else
	{
		if (debug_sanityCheck__compareDB (ctxSanity, baseFolder))
		{
			logger->log (eTextColor::green, "success\n");
			
			//delete db sanity
			sprintf_s (s, sizeof(s), "%s/%s", baseFolder, DB_NAME);
			fs::fileDelete(s);
		}
		else
		{
			save_dependencies_report (baseFolder, DB_NAME);
			logger->log (eTextColor::red, "FAILED\n");
		}
	}
	asset2::dbcontext_close (ctxSanity);

	return ret;
}

//***********************************
bool Builder::debug_sanityCheck__compareDB (DBContext &ctxSanity, const char *baseFolder)
{
	bool ret = true;

	//ora faccio un po' di verifiche tra i 2 DB
	DBContext ctx;
	if (!asset2::dbcontext_open (baseFolder, &ctx))
	{
		logger->err ("can't open regular DB\n");
		return false;
	}
    
	char s[1024];
	sprintf_s (s, sizeof(s), "SELECT UID FROM " GOS_ASSET2__TABLE_ASSET_LIST " ORDER BY UID");
	if (!debug_sanityCheck__cmp_table (ctxSanity, ctx, s, GOS_ASSET2__TABLE_ASSET_LIST))
		ret = false;

	sprintf_s (s, sizeof(s), "SELECT UID,childUID FROM " GOS_ASSET2__TABLE_DEPENDS " ORDER BY UID");
	if (!debug_sanityCheck__cmp_table (ctxSanity, ctx, s, GOS_ASSET2__TABLE_DEPENDS))
		ret = false;

	sprintf_s (s, sizeof(s), "SELECT UID,type,abspath FROM " GOS_ASSET2__TABLE_RES " ORDER BY UID");
	if (!debug_sanityCheck__cmp_table (ctxSanity, ctx, s, GOS_ASSET2__TABLE_RES))
		ret = false;

	sprintf_s (s, sizeof(s), "SELECT UID,childUID FROM " GOS_ASSET2__TABLE_DEPENDS_RUNTIME " ORDER BY UID");
	if (!debug_sanityCheck__cmp_table (ctxSanity, ctx, s, GOS_ASSET2__TABLE_DEPENDS_RUNTIME))
		ret = false;

	sprintf_s (s, sizeof(s), "SELECT UID,UID_ini,line,UID_asset FROM " GOS_ASSET2__TABLE_VIRTUAL_ASSET " ORDER BY UID,UID_ini,line,UID_asset");
	if (!debug_sanityCheck__cmp_table (ctxSanity, ctx, s, GOS_ASSET2__TABLE_VIRTUAL_ASSET))
		ret = false;

	asset2::dbcontext_close (ctx);
	return ret;
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

bool Builder::debug_sanityCheck__cmp_table (DBContext &ctx_sanity, DBContext &ctx, const char *sql, const char *tableName) const
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


//****************************** 
bool Builder::rebuildAll (const char *baseFolder)
{
	char s[1024];

	//del del database
	sprintf_s (s, sizeof(s), "%s/" GOS_ASSET2__DEFAULT_DB_NAME "", baseFolder);
	fs::fileDelete(s);

	DBContext ctx;
	if (!asset2::dbcontext_open (baseFolder, &ctx))
		return false;

	//del degli asset
	fs::folderDeleteAllFileRecursively (ctx.folder_assets_bin, eFolderDeleteMode::doNotDeleteAnyFolder);

	//build
	logger->log ("rebuild all...\n");
	const bool ret = priv_build (ctx, true);
	asset2::dbcontext_close (ctx);
	return ret;	
}

//****************************** 
bool Builder::build (const char *baseFolder)
{
	DBContext ctx;
	if (!asset2::dbcontext_open (baseFolder, &ctx))
		return false;

	logger->log ("building %s\n", baseFolder);
	const bool ret = priv_build (ctx, true);
	asset2::dbcontext_close (ctx);
	return ret;	
}

//****************************** 
bool Builder::priv_build (DBContext &ctx,  bool bDoCreateAssetFile)
{
	gos::DateTime dt;
	dt.setNow_UTC();
	buildTime_UTC = dt.getAsNiceU64();


	HashedStringList listof_gosAssetd_toBeRebuilt(localAllocator, 256);
	UniqueUIDList listof_possibile_assets_to_be_deleted(localAllocator, 256);
	UniqueUIDList listof_deleted_gosassetd(localAllocator, 256);
	bool ret = true;

	//verifico lo stato di tutte le risorse presenti nel DB per vedere se qualcuna di queste
	//e' stata modificata o eliminata.
	//Alla fine, l'output di questo passo e' un elenco di gosasset_d da rebuildare
	logger->log ("\nScanning known resources...\n");
	logger->incIndent();
	{
		//scanno gli .gosasset_d presenti su HD per vedere se ne ce sono di nuovi
		ret = priv_gosassetd_scan_folder (ctx, ctx.folder_assets_src, &listof_gosAssetd_toBeRebuilt);

		//scanno tutte le risorse gia' presenti nel DB
		priv_resource_scan_DB (ctx, &listof_gosAssetd_toBeRebuilt, &listof_deleted_gosassetd, &listof_possibile_assets_to_be_deleted);

		logger->log ("finished\n");
	}
	logger->decIndent();
	if (!ret)
		return false;

	//dalla lista dei possibili concrete-asset da deletare, verifico quali sono effettivamente da cancellare
	UniqueUIDList listof_deleted_assets(localAllocator, 256);
	listof_possibile_assets_to_be_deleted.forEach( [&ctx, &listof_deleted_assets](u32 index, const UID uid) {
		if (!asset_is_still_in_use(ctx, uid))
		{
			listof_deleted_assets.insertIfNotExists(uid);
			asset_delete(ctx, uid);
		}
		return true;
	});


	//ora ho una lista di gosasset_d che devo rebuildare, la processo
	UniqueUIDList listof_builtAssets(localAllocator, 256);
	logger->log ("\nList of .gosasset_d to be rebuilt:\n");
	{
		logger->incIndent();
		if (0 == listof_gosAssetd_toBeRebuilt.getNElem())
		{
			logger->log ("Nothing to do\n");
			logger->decIndent();
		}
		else
		{
			auto list = listof_gosAssetd_toBeRebuilt._queryList();
			for (u32 i=0; i< list->getNElem(); i++)
			{
				UID uid = list->queryElem(i).key;
				const char *absFilename = list->queryElem(i).value.getBuffer();

				logger->log ("[%-12s] %016" PRIX64 " %s\n", asset2::enumToString (uid.getResourceType()), uid._uid, absFilename);
			}
			logger->decIndent();

			//build
			for (u32 i=0; i< list->getNElem(); i++)
			{
				UID uid = list->queryElem(i).key;
				const char *absFilename = list->queryElem(i).value.getBuffer();
				logger->log ("\nbuilding %016" PRIX64 " %s\n", uid._uid, absFilename);
				logger->incIndent();
				ret = priv_gosassetd_build(ctx, absFilename, &listof_builtAssets);
				logger->decIndent();

				if (!ret)
					break;
			}
		}		
	}
	
	if (ret)
	{
		//clean up del DB
		if (listof_deleted_assets.getNElem())
		{
			logger->log ("\nlist of deleted assets:\n");
			logger->incIndent();
			listof_deleted_assets.forEach( [logger=this->logger](u32 index, const UID uid) {
				logger->log ("[%-12s] %016" PRIX64 "\n", asset2::enumToString (uid.getAssetType()), uid._uid);
				return true;
			});
			logger->decIndent();
		}

		if (listof_builtAssets.getNElem())
		{
			logger->log ("\nlist of built assets:\n");
			logger->incIndent();
			listof_builtAssets.forEach( [logger=this->logger](u32 index, const UID uid) {
				logger->log ("[%-12s] %016" PRIX64 "\n", asset2::enumToString (uid.getAssetType()), uid._uid);
				return true;
			});
			logger->decIndent();
		}
		
	}
	//fine
	return ret;
}

/****************************** 
 * Recupero tutte le risorse storate nel DB e per ciascuna di queste verifico se sono state modificate o eliminate.
 */
void Builder::priv_resource_scan_DB (DBContext &ctx, HashedStringList *out_listof_gosassetd_toRebuild,  UniqueUIDList *out_listof_deleted_gosassetd, UniqueUIDList *out_listOfPossibileAssetsToBeDeleted) const
{
	char s[1024];
    sprintf_s (s, sizeof(s), "SELECT UID,lastTimeMod,abspath FROM " GOS_ASSET2__TABLE_RES " ORDER BY abspath");

	db::RST rst;
	if (!db::query (ctx.db, s, &rst))
	{
		logger::err ("Builder::priv_resource_scan_DB => invalid query\n");
		return;
	}

	while (rst.fetchRow())
	{
		sResListElem elem;
		elem.uid._uid = rst.getValAsU64(0);;
		elem.lastTimeModified = rst.getValAsU64(1);
		sprintf_s (elem.abspath, sizeof(elem.abspath), "%s", rst.getVal(2));
		elem.status = eBuildStatus::UNCHANGED;
		
		if (!fs::fileExists (elem.abspath))
		{
			//la risorsa e' stata eliminata
			elem.status = eBuildStatus::DELETED;
			if (elem.uid.isAResourceOfType(eResType::gosasset_d))
				out_listof_deleted_gosassetd->insertIfNotExists(elem.uid);
		}
		else
		{
			//vediamo se la risorsa e' stata modificata
			const u64 lastTimeMod = fs::fileGetLastTimeModified_UTC_niceu64(elem.abspath);
			if (lastTimeMod != elem.lastTimeModified)
			{
				elem.lastTimeModified = lastTimeMod;
				elem.status = eBuildStatus::MODIFIED;

				if (elem.uid.isAResourceOfType(eResType::gosasset_d))
					out_listof_gosassetd_toRebuild->insertIfNotExists (elem.uid, elem.abspath);
			}
		}

		if (eBuildStatus::UNCHANGED != elem.status)
		{
			priv_printResListElem(elem);
			
			//Cerco tutte le risorse che dipendono direttamente o indirettamente da questa e le elimino dal DB.
			//L'idea e' che se una risorsa e' stata modificata/eliminata, allora tutte quelle che ne necessitano devono comunque essere ricreate
			//perche' le dipendenze porebbero essere cambiate.
			//Elimino tutte queste risorse dal DB e, nel fare questo, mi segno quali risorse di tipo .gosasset_d sono coinvolte in moda da rebuildare
			//ed eventualmente ripopolare il DB
			//Le risorse di tipo gosasset_d le marco come da rebuildare
			UniqueUIDList uidList(localAllocator, 1024);
			dependency_get_requireBy_list (ctx, elem.uid, true, &uidList);

			uidList.forEach ( [&ctx, out_listof_gosassetd_toRebuild, out_listof_deleted_gosassetd, out_listOfPossibileAssetsToBeDeleted](u32 index, const UID uid) {
				if (uid.isAResource())
				{
					if (eResType::gosasset_d == uid.getResourceType())
					{
						char s[1024];
						if (res_get_info (ctx, uid, s, sizeof(s), NULL, NULL))
						{
							if (out_listof_deleted_gosassetd->insertIfNotExists(uid))
								out_listof_gosassetd_toRebuild->insertIfNotExists (uid, s);
						}
					}
					
					res_delete (ctx, uid);
				}
				else if (uid.isVirtualAsset())
				{
					UID uid_ini;
					UID uid_concrete_asset;
					if (virtasset_get_info (ctx, uid, &uid_ini, &uid_concrete_asset))
					{
						char s[1024];
						if (res_get_info (ctx, uid_ini, s, sizeof(s), NULL, NULL))
						{
							if (out_listof_deleted_gosassetd->insertIfNotExists(uid_ini))
								out_listof_gosassetd_toRebuild->insertIfNotExists (uid_ini, s);
							out_listOfPossibileAssetsToBeDeleted->insertIfNotExists (uid_concrete_asset);
						}
					}

					virtasset_delete (ctx, uid);					
				}
				else
				{
					//in questa lista non ci possono essere dei concrete-asset perche' i
					//concrete sono "required" solo dai virtual-asset
					assert (uid.isAnAsset());
					DBGBREAK;
				}
				return true;
			});

			res_delete (ctx, elem.uid);

		}
	}

}

//****************************** 
bool Builder::priv_gosassetd_scan_folder (DBContext &ctx, const char *folder_path, HashedStringList *out_listof_gosassetd_toRebuild) const
{
	//Cerca tutte i file .gosasset_d nelle cartelle e sottocartelle del progetto
	bool ret = true;

	gos::FileFind ff;
	if (fs::findFirst (&ff, folder_path, "*.gosasset_d", eFileFindMode::only_file))
	{
		gos::StringList stringList(localAllocator, 4096);

		do
		{
			char absFilename[1024];
			sprintf_s (absFilename, sizeof(absFilename), "%s/%s", folder_path, fs::findGetFileName(ff));
			ret = priv_gosassetd_scan_folder_parse (ctx, absFilename, out_listof_gosassetd_toRebuild);
			if (!ret)
				break;
		} while (fs::findNext(ff));
		fs::findClose (ff);
	}

	//rifaccio il giro in cerca dei subfolder
	if (ret && fs::findFirst (&ff, folder_path, "*.*", eFileFindMode::only_folder))
	{
		do
		{
			char s[1024];
			sprintf_s (s, sizeof(s), "%s/%s", folder_path, fs::findGetFileName(ff));
			ret = priv_gosassetd_scan_folder(ctx, s, out_listof_gosassetd_toRebuild);
			if (!ret)
				break;
		} while (fs::findNext(ff));
		fs::findClose (ff);
	}

	return ret;
}

//****************************** 
bool Builder::priv_gosassetd_scan_folder_parse (DBContext &ctx, const char *absFilenameIN, HashedStringList *out_listof_gosassetd_toRebuild) const
{
	assert (fs::isPathAbsolute(absFilenameIN));

	//ho trovato un .gosasset_d in una della directory del progetto.
	//Verifico se era gia' nel DB o se e' nuovo
	UID uid;
	if (res_exists (ctx, eResType::gosasset_d, absFilenameIN, &uid))
		return true;

	//dato che non era gia' nel DB, deve per forza essere una risorsa nuova
	res_createUID (eResType::gosasset_d, absFilenameIN, &uid);
	out_listof_gosassetd_toRebuild->insertIfNotExists(uid, absFilenameIN);


	//info a video
	{
		sResListElem elem;
		elem.uid = uid;
		elem.lastTimeModified = fs::fileGetLastTimeModified_UTC_niceu64(absFilenameIN);
		sprintf_s (elem.abspath, sizeof(elem.abspath), "%s", absFilenameIN);
		elem.status = eBuildStatus::NEW;
		priv_printResListElem(elem);
	}

	return true;
}

//****************************** 
BuilderInterface* Builder::priv_findBuilderByClassName (const char *assetClassName) const
{
	for (u32 i=0; i<NUM_MAX_BUILDERS; i++)
	{
		if (NULL == builderList[i])
			continue;

		if (0 == strcmp(asset2::enumToString(builderList[i]->getAssetType()), assetClassName))
			return builderList[i];
	}
	return NULL;
}

//****************************** 
bool Builder::priv_extractAllInludePaths (const char *absFilenameIN, gos::StringList *out) const
{
	bool ret = true;
	u32 fsize=0;
    u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), absFilenameIN, &fsize);
    if (NULL == buffer)
	{
		logger->err ("can't open %s\n", absFilenameIN);
		return false;
	}

	string::IncludeDetector det;
	const u32 n = det.parse (buffer, fsize);
	for (u32 i=0; i<n; i++)
	{
		char s[1024];
		det.getResultAsString (buffer, i, s, sizeof(s));

		//i path degli include possono essere relativi
		char absIncludeFilename[1024];
		if (fs::isPathAbsolute(s))
			sprintf_s (absIncludeFilename, sizeof(absIncludeFilename), "%s", s);
		else
		{
			fs::extractFilePathWithSlash (absFilenameIN, absIncludeFilename, sizeof(absIncludeFilename));
			strcat_s (absIncludeFilename, sizeof(absIncludeFilename), s);
			fs::pathSanitizeInPlace (absIncludeFilename);
		}

		out->add(absIncludeFilename);
	}

	GOSFREE_SCRAP (buffer);
	return ret;
}

//****************************** 
void Builder::priv_fromDirectiveNameToAssetClassName (const char *directiveName, char *out_asseetClassName, u32 sizeof_out) const
{
	assert (directiveName[0] == '@');
	sprintf_s (out_asseetClassName, sizeof_out, "%s", &directiveName[1]);
	u32 i=0;
	while (out_asseetClassName[i] != 0x00)
	{
		if ('@' == out_asseetClassName[i])
		{
			out_asseetClassName[i] = 0x00;
			return;
		}
		else
			i++;
	}	
}

//****************************** 
bool Builder::priv_gosassetd_build (DBContext &ctx, const char *absFilename, UniqueUIDList *out_listOfBuiltAssets)
{
	gos::IniFile ini;
	if (!ini.loadAndParse (absFilename))
	{
		logger->err ("error parsing file\n");
		return false;
	}

	//se non esisto, mi addo al DB
	UID uid_of_iniFile;
	if (!res_exists (ctx, eResType::gosasset_d, absFilename, &uid_of_iniFile))
	{
		if (!res_insert (ctx, eResType::gosasset_d, absFilename, fs::fileGetLastTimeModified_UTC_niceu64(absFilename), &uid_of_iniFile))
			return false;
	}

	UniqueStringList listof_knownRTname(localAllocator, 2048);
	u32 nextAnonymAssetName = 0;
	return priv_gosassetd_buildSection (ctx, nextAnonymAssetName, listof_knownRTname, absFilename, uid_of_iniFile, ini.getRoot(), out_listOfBuiltAssets);
}

//****************************** 
bool Builder::priv_gosassetd_buildSection (DBContext &ctx, u32 &in_out_nextAnonymAssetName, UniqueStringList &in_out_listof_knownRTname, const char *absFilename, UID uid_of_iniFile, gos::IniFileSection *section, UniqueUIDList *out_listOfBuiltAssets)
{
	for (u32 iSec=0; iSec<section->getNSubsection(); iSec++)
	{
		//deve essere di tipo direttiva, altrimenti e' un errore
		gos::IniFileSection *sub = section->getSubsectionByIndex(iSec);
		const char *subName = sub->name.getBuffer();
		if (subName[0] != '@')
		{
			logger->err ("line %d => invalid subsection, it must start with @\n", sub->getLineStarted());
			return false;
		}    
	
		//verifico che tipo di asset sta descrivendo.
		char assetClass[64];
		priv_fromDirectiveNameToAssetClassName (subName, assetClass, sizeof(assetClass));

		//e verifico che ci sia un builder adeguato
		BuilderInterface* builder = priv_findBuilderByClassName(assetClass);
		if (NULL == builder)
		{
			logger->err ("line %d => can't find a builder for asset of type '%s'\n", sub->getLineStarted(), assetClass);
			return false;
		}

		//se non ha un runtimeName per l'asset che descrive, gliene assegno uno d'ufficio
		char assetRuntimeName[128];
		if (!sub->exists ("__value"))
		{
			sprintf_s (assetRuntimeName, sizeof(assetRuntimeName), "__%" PRIu64 "_%06d", uid_of_iniFile._uid, in_out_nextAnonymAssetName++);
			sub->set ("__value", assetRuntimeName);
		}			
		else
			sub->get ("__value", assetRuntimeName, sizeof(assetRuntimeName));

		//buildo
		logger->log ("line %d, %s : %s\n", sub->getLineStarted(), assetClass, assetRuntimeName);
		logger->incIndent();
		bool ret = false;
		while (1)
		{
			//se e' il nome di una sezione con un runtime-name ma nessun parametro, vuol dire che e' una direttiva
			//gia' risolta che punta ad un runtime-name ben specifico.
			//In sostanza, e' una sottodirettiva di una direttiva di livello superiore (es: @vtx_shader all'interno di @pipe)
			//Il rt-name deve essermi gia' noto
			if (1 == sub->getNIdentifier() && assetRuntimeName[0]!='_' && assetRuntimeName[1]!='_')
			{
				if (!in_out_listof_knownRTname.exists(assetRuntimeName))
				{
					ret = false;
					logger->log (eTextColor::red, "rt-name %s is unknown\n", assetRuntimeName);
				}
				else
				{
					ret = true;
					logger->log ("skip because is an inline-directive\n");
				}
				break;
			}

			//asset con depth > 1 possono avere delle subsection inline, che devo risolvere prima
			//di poter buildare l'asset
			if (sub->getNSubsection())
			{
				if (!priv_gosassetd_buildSection (ctx, in_out_nextAnonymAssetName, in_out_listof_knownRTname, absFilename, uid_of_iniFile, sub, out_listOfBuiltAssets))
					break;
			}

			sBuildResult result;
			builder->setLogger(logger);
			if (!builder->build (ctx, buildTime_UTC, absFilename, uid_of_iniFile, sub, true, &result))
				break;

			//aggiungo rt-name alla lista dei nomi noti da questo file
			in_out_listof_knownRTname.add (assetRuntimeName);
				
			//report a video del risultato della build
			eTextColor color = eTextColor::green;
			if (eBuildResult::was_already_built == result.result)
				color = eTextColor::darkBlue;
			logger->log (color, "[%-17s] %016" PRIX64 " [%016" PRIX64 "]\n", asset2::enumToString(result.result), result.uid_virtual_asset._uid, result.uid_concrete_asset._uid);

			//calcolo e scrivo le dipendenze runtime di questo asset
			//Per "dipendenze runtime" intendo una lista di altri asset (e non risorse) dai quali questo asset dipende
			if (eBuildResult::just_built == result.result)
			{
				out_listOfBuiltAssets->insertIfNotExists (result.uid_concrete_asset);

				UniqueUIDList   hashList1 (localAllocator, 256);
				dependency_get_dependecies_list (ctx, result.uid_concrete_asset, true, &hashList1);

				auto list = hashList1._queryList();
				for (u32 i=0; i<list->getNElem(); i++)
				{
					const UID childUID = list->queryElem(i);
					if (childUID.isAnAsset())
						dependencyRT_add (ctx, result.uid_concrete_asset, childUID);
				}
			}

				
			//fine while (1)
			ret = true;
			break;
		}
		logger->decIndent();

		if (!ret)
			return false;
	}

	return true;
}