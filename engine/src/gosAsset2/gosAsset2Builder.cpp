#include "gosAsset2Builder.h"
#include "gosAsset2.h"
#include "gos.h"
#include "string/gosStringIncludeDetector.h"
#include "builders/gosAsset2Builder_shader.h"
#include "builders/gosAsset2Builder_pipe.h"
#include "builders/gosAsset2Builder_tex2D.h"
#include "builders/gosAsset2Builder_model3d.h"

using namespace gos;
using namespace gos::asset2;

//******************************
Builder::Builder(gos::GPU *gpuIN)
{
	gpu = gpuIN;
	localAllocator = gos::getSysHeapAllocator();
	logger = &loggerStdout;

	build_result_list.setup (localAllocator, 256);

	memset(builderList, 0, sizeof(builderList));
	add_builder<Builder_vtxShader>();
	add_builder<Builder_pxlShader>();
	add_builder<Builder_pipe>();
	add_builder<Builder_tex2D>();
	//add_builder<Builder_glb>();
	add_builder<Builder_model3d>();
}

//******************************
Builder::~Builder()
{
	for (u32 i = 0; i < NUM_MAX_BUILDERS; i++)
	{
		if (NULL != builderList[i])
		{
			builderList[i]->deinitOnce();
			GOSDELETE(localAllocator, builderList[i]);
		}
	}
}

//***********************************
bool Builder::priv_addBuilder(BuilderInterface *builder)
{
	assert(NULL != builder);

	const u32 index = static_cast<u8>(builder->getAssetType());
	assert(index < NUM_MAX_BUILDERS);

	if (NULL == builderList[index])
	{
		builderList[index] = builder;
		builder->initOnce(gpu);
		return true;
	}

	logger->err("Builder::priv_addBuilder() => a builder for res %s already exists\n", asset2::enumToString(builder->getAssetType()));
	return false;
}

//***********************************
const char *Builder::enumToString(eBuildStatus s) const
{
	switch (s)
	{
	default:
		DBGBREAK;
		return "!!ERR::eBuildStatus!!";
	case eBuildStatus::NEW:
		return "NEW";
	case eBuildStatus::MODIFIED:
		return "MODIFIED";
	case eBuildStatus::DELETED:
		return "DELETED";
	case eBuildStatus::UNCHANGED:
		return "UNCHANGED";
	}
}

//***********************************
void Builder::priv_printResListElem(const sResListElem &elem) const
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
		// continue;
		break;
	}

	gos::DateTime dt;
	dt.setFromNiceU64(elem.lastTimeModified);

	char lastTimeMod[64];
	dt.formatAs_YYYYMMDDHHMMSS(lastTimeMod, sizeof(lastTimeMod));

	logger->log(color, "%-10s [%-12s] %016" PRIX64 " % 20s %s\n",
				enumToString(elem.status),
				asset2::enumToString(elem.uid.getResourceType()),
				elem.uid._uid,
				lastTimeMod,
				elem.abspath);
}

//***********************************
void Builder::priv_printResList(const ResList &list) const
{
	for (u32 i = 0; i < list.getNElem(); i++)
	{
		priv_printResListElem(list(i));
	}
}

//***********************************
bool Builder::debug_sanityCheck(const char *baseFolder)
{
	static const char SANITY_DB_NAME[] = {"sanitycheck.sqlite3"};

	logger = &loggerStdout;

	char log_folder[512];
	sprintf_s(log_folder, sizeof(log_folder), "%s/%s.log", baseFolder, SANITY_DB_NAME);
	fs::folderDeleteAllFileRecursively(log_folder, eFolderDeleteMode::deleteAlsoTheSubfolderAndTheMainFolder);
	loggerStdout.enableFileLogging(log_folder);

	logger->log(eTextColor::yellow, "\n\n=== RUNNING SANITY CHECK....\n");
	logger->inc_indent();

	// faccio un rebuild all usando un nome db specifico
	char s[1024];
	sprintf_s(s, sizeof(s), "%s/%s", baseFolder, SANITY_DB_NAME);
	fs::fileDelete(s);

	DBContext ctxSanity;
	if (!asset2::dbcontext_open_ex(baseFolder, SANITY_DB_NAME, true, &ctxSanity))
	{
		logger->err("Can't create DB\n");
		return false;
	}

	logger->log("building...\n");
	loggerStdout.disableStdouLogging();
	bool ret = priv_build(ctxSanity, false, false);
	loggerStdout.enableStdouLogging();

	if (!ret)
	{
		logger->log(eTextColor::red, "sanity build FAILED\n");
	}
	else
	{
		logger->log("comparing...\n");
		ret = debug_sanityCheck__compareDB(ctxSanity, baseFolder);
	}
	asset2::dbcontext_close(ctxSanity);

	// pulizia finale
	if (ret)
	{
		logger->log(eTextColor::green, "success\n");
		loggerStdout.disableFileLogging();

		// delete db sanity
		sprintf_s(s, sizeof(s), "%s/%s", baseFolder, SANITY_DB_NAME);
		fs::fileDelete(s);

		// delete fs::folderDelete (log_folder);
		fs::folderDeleteAllFileRecursively(log_folder, eFolderDeleteMode::deleteAlsoTheSubfolderAndTheMainFolder);
	}
	else
	{
		save_dependencies_report(baseFolder, SANITY_DB_NAME);
		save_asset_manifest(baseFolder, SANITY_DB_NAME);
		logger->log(eTextColor::red, "FAILED\n");
	}

	return ret;
}

//***********************************
bool Builder::debug_sanityCheck__compareDB(DBContext &ctxSanity, const char *baseFolder)
{
	bool ret = true;

	// ora faccio un po' di verifiche tra i 2 DB
	DBContext ctx;
	if (!asset2::dbcontext_open(baseFolder, false, &ctx))
	{
		logger->err("can't open regular DB\n");
		return false;
	}

	char s[1024];
	sprintf_s(s, sizeof(s), "SELECT UID FROM " GOS_ASSET2__TABLE_ASSET_LIST " ORDER BY UID");
	if (!debug_sanityCheck__cmp_table(ctxSanity, ctx, s, GOS_ASSET2__TABLE_ASSET_LIST))
		ret = false;

	sprintf_s(s, sizeof(s), "SELECT UID,childUID FROM " GOS_ASSET2__TABLE_DEPENDS " ORDER BY UID");
	if (!debug_sanityCheck__cmp_table(ctxSanity, ctx, s, GOS_ASSET2__TABLE_DEPENDS))
		ret = false;

	sprintf_s(s, sizeof(s), "SELECT UID,type,abspath FROM " GOS_ASSET2__TABLE_RES " ORDER BY UID");
	if (!debug_sanityCheck__cmp_table(ctxSanity, ctx, s, GOS_ASSET2__TABLE_RES))
		ret = false;

	sprintf_s(s, sizeof(s), "SELECT UID,childUID FROM " GOS_ASSET2__TABLE_DEPENDS_RUNTIME " ORDER BY UID");
	if (!debug_sanityCheck__cmp_table(ctxSanity, ctx, s, GOS_ASSET2__TABLE_DEPENDS_RUNTIME))
		ret = false;

	sprintf_s(s, sizeof(s), "	");
	if (!debug_sanityCheck__cmp_table(ctxSanity, ctx, s, GOS_ASSET2__TABLE_VIRTUAL_ASSET))
		ret = false;

	asset2::dbcontext_close(ctx);
	return ret;
}

//***********************************
u32 Builder::debug_sanityCheck__count(db::RST &rst) const
{
	u32 ret = 0;
	while (rst.fetchRow())
		ret++;

	rst.rewind();
	return ret;
}

bool Builder::debug_sanityCheck__compare(db::RST &rst1, db::RST &rst2, u32 rowIndex) const
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

	return (0 == strcmp(v1, v2));
}

bool Builder::debug_sanityCheck__cmp_table(DBContext &ctx_sanity, DBContext &ctx, const char *sql, const char *tableName) const
{
	db::RST rst1;
	db::query(ctx.db, sql, &rst1);

	db::RST rst2;
	db::query(ctx_sanity.db, sql, &rst2);

	const u32 n1 = debug_sanityCheck__count(rst1);
	const u32 n2 = debug_sanityCheck__count(rst2);

	if (n1 != n2)
	{
		logger->log("table '%s' => num record differs (%d vs %d)\n", tableName, n1, n2);
		return false;
	}

	bool ret = true;
	u32 rowNum = 0;
	while (rst1.fetchRow())
	{
		rst2.fetchRow();
		rowNum++;

		for (u32 i = 0; i < rst1.getNumCols(); i++)
		{
			if (!debug_sanityCheck__compare(rst1, rst2, i))
			{
				ret = false;

				const char *v1 = rst1.getVal(i);
				const char *v2 = rst2.getVal(i);
				logger->log("table '%s', row %d => value for col '%s' differs  [%s] [%s]\n", tableName, rowNum, rst1.getColName(i), v1, v2);
			}
		}
	}
	return ret;
}

//******************************
bool Builder::rebuild_all(const char *baseFolder, bool bVerbose)
{
	if (bVerbose)
		logger = &loggerStdout;
	else
		logger = &loggerNull;

	char s[1024];

	// del del database
	sprintf_s(s, sizeof(s), "%s/" GOS_ASSET2__DEFAULT_DB_NAME "", baseFolder);
	fs::fileDelete(s);

	DBContext ctx;
	if (!asset2::dbcontext_open(baseFolder, true, &ctx))
		return false;

	// del degli asset
	fs::folderDeleteAllFileRecursively(ctx.folder_assets_bin, eFolderDeleteMode::doNotDeleteAnyFolder);

	// build
	logger->log("rebuild all...\n");
	const bool ret = priv_build(ctx, true, false);
	asset2::dbcontext_close(ctx);
	return ret;
}

//******************************
bool Builder::build(const char *baseFolder, bool bVerbose)
{
	if (bVerbose)
		logger = &loggerStdout;
	else
		logger = &loggerNull;

	bool ret = false;

	// faccio un backup del DB
	char s[512];
	char backupDB[512];
	{
		sprintf_s(s, sizeof(s), "%s/" GOS_ASSET2__DEFAULT_DB_NAME "", baseFolder);
		sprintf_s(backupDB, sizeof(backupDB), "%s.backup", s);
		fs::fileCopy(s, backupDB);
	}

	DBContext ctx;
	if (asset2::dbcontext_open(baseFolder, true, &ctx))
	{
		logger->log("building %s\n", baseFolder);
		ret = priv_build(ctx, true, true);
		asset2::dbcontext_close(ctx);
	}

	if (!ret)
	{
		logger->log(eTextColor::yellow, "\n\nrestoring previous DB\n");

		sprintf_s(s, sizeof(s), "%s/" GOS_ASSET2__DEFAULT_DB_NAME "", baseFolder);
		fs::fileDelete(s);
		fs::fileCopy(backupDB, s);
	}
	fs::fileDelete(backupDB);

	return ret;
}

//******************************
bool Builder::priv_build (DBContext &ctx, bool bDoCreateAssetFile, bool bGenerateListOfUpdatedUID)
{
	if (bGenerateListOfUpdatedUID)
		build_result_list.reset();

	gos::DateTime dt;
	dt.setNow_UTC();

	HashedStringList listof_gosAssetd_toBeRebuilt(localAllocator, 256);
	UniqueUIDList listof_possibile_concrete_assets_to_be_deleted(localAllocator, 256);
	UniqueUIDList listof_possibile_resources_to_be_deleted(localAllocator, 256);
	UniqueUIDList listof_deleted_gosassetd(localAllocator, 256);
	
	bool ret = true;

	// verifico lo stato di tutte le risorse presenti nel DB per vedere se qualcuna di queste
	// e' stata modificata o eliminata.
	// Alla fine, l'output di questo passo e' un elenco di gosasset_d da rebuildare
	logger->log("\nScanning known resources...\n");
	logger->inc_indent();
	{
		// scanno gli .gosasset_d presenti su HD per vedere se ne ce sono di nuovi
		ret = priv_gosassetd_scan_folder(ctx, ctx.folder_assets_src, &listof_gosAssetd_toBeRebuilt);

		// scanno tutte le risorse gia' presenti nel DB
		if (ret)
			ret = priv_resource_scan_DB(ctx, &listof_gosAssetd_toBeRebuilt, &listof_deleted_gosassetd, &listof_possibile_concrete_assets_to_be_deleted, &listof_possibile_resources_to_be_deleted);

		logger->log("finished\n");
	}
	logger->dec_indent();
	if (!ret)
		return false;

	// dalla lista dei possibili concrete-asset da deletare, verifico quali sono effettivamente da cancellare
	UniqueUIDList listof_deleted_assets(localAllocator, 256);
	listof_possibile_concrete_assets_to_be_deleted.forEach([&ctx, &listof_deleted_assets](u32 index, const UID uid)
	{
		if (!asset_is_still_in_use(ctx, uid))
		{
			listof_deleted_assets.insertIfNotExists(uid);
			asset_delete(ctx, uid);
		}
		return true;
	});

	// ora ho una lista di gosasset_d che devo rebuildare, la processo
	UniqueUIDList listof_builtAssets(localAllocator, 256);
	logger->log("\nList of .gosasset_d to be rebuilt:\n");
	{
		logger->inc_indent();
		if (0 == listof_gosAssetd_toBeRebuilt.getNElem())
		{
			logger->log("Nothing to do\n");
			logger->dec_indent();
		}
		else
		{
			auto list = listof_gosAssetd_toBeRebuilt._queryList();
			for (u32 i = 0; i < list->getNElem(); i++)
			{
				const UID uid = list->queryElem(i).key;
				const char *absFilename = list->queryElem(i).value.getBuffer();
				if (fs::fileExists(absFilename))
					logger->log("[%-12s] %016" PRIX64 " %s\n", asset2::enumToString(uid.getResourceType()), uid._uid, absFilename);
			}
			logger->dec_indent();

			// build
			for (u32 i = 0; i < list->getNElem(); i++)
			{
				const UID uid = list->queryElem(i).key;
				const char *absFilename = list->queryElem(i).value.getBuffer();
				if (fs::fileExists(absFilename))
				{
					logger->log("\nbuilding %016" PRIX64 " %s\n", uid._uid, absFilename);
					logger->inc_indent();
					ret = priv_gosassetd_build(ctx, bDoCreateAssetFile, absFilename, &listof_builtAssets);
					logger->dec_indent();
				}
				if (!ret)
					break;
			}
		}
	}

	if (ret)
	{
		//elimino dal DB eventuali risorse che non servono piu'
		if (listof_possibile_resources_to_be_deleted.getNElem())
		{
			logger->log("\nlist of removed resources:\n");
			logger->inc_indent();
			listof_possibile_resources_to_be_deleted.forEach([&ctx, logger=this->logger](u32 index, const UID uid)
			{
				assert (uid.isAResource());
				if (!res_is_still_in_use(ctx, uid))
				{
					logger->log ("removing [%-12s] %016" PRIX64 " because is no longer needed\n", asset2::enumToString (uid.getResourceType()), uid._uid);
					res_delete(ctx, uid);
				}
				return true;
			});
			logger->dec_indent();
		}

		// clean up del DB
		if (listof_deleted_assets.getNElem())
		{
			logger->log("\nlist of deleted assets:\n");
			logger->inc_indent();
			listof_deleted_assets.forEach([bGenerateListOfUpdatedUID, logger = this->logger, &build_result_list=this->build_result_list](u32 index, const UID uid)
			{
				logger->log ("[%-12s] %016" PRIX64 "\n", asset2::enumToString (uid.getAssetType()), uid._uid);
				if (bGenerateListOfUpdatedUID)
					build_result_list.insertIfNotExists(uid);
				return true; 
			});
			logger->dec_indent();
		}

		if (listof_builtAssets.getNElem())
		{
			logger->log("\nlist of built assets:\n");
			logger->inc_indent();
			listof_builtAssets.forEach([bGenerateListOfUpdatedUID, logger = this->logger, &build_result_list=this->build_result_list](u32 index, const UID uid)
			{
				logger->log ("[%-12s] %016" PRIX64 "\n", asset2::enumToString (uid.getAssetType()), uid._uid);
				if (bGenerateListOfUpdatedUID)
					build_result_list.insertIfNotExists(uid);
				return true;
			});
			logger->dec_indent();
		}
	}
	// fine
	return ret;
}

/******************************
 * Recupero tutte le risorse storate nel DB e per ciascuna di queste verifico se sono state modificate o eliminate.
 */
bool Builder::priv_resource_scan_DB (DBContext &ctx, HashedStringList *out_listof_gosassetd_toRebuild, UniqueUIDList *out_listof_deleted_gosassetd, UniqueUIDList *out_listOfPossibileConcreteAssetsToBeDeleted, UniqueUIDList *out_listOfPossibileResourceToBeDeleted) const
{
	char s[1024];
	sprintf_s(s, sizeof(s), "SELECT UID,lastTimeMod,abspath FROM " GOS_ASSET2__TABLE_RES " ORDER BY abspath");

	db::RST rst;
	if (!db::query(ctx.db, s, &rst))
	{
		logger::err("Builder::priv_resource_scan_DB => invalid query\n");
		return false;
	}

	while (rst.fetchRow())
	{
		sResListElem elem;
		elem.uid._uid = rst.getValAsU64(0);
		;
		elem.lastTimeModified = rst.getValAsU64(1);
		sprintf_s(elem.abspath, sizeof(elem.abspath), "%s", rst.getVal(2));
		elem.status = eBuildStatus::UNCHANGED;

		if (!fs::fileExists(elem.abspath))
		{
			// la risorsa e' stata eliminata
			elem.status = eBuildStatus::DELETED;
			if (elem.uid.isAResourceOfType(eResType::gosasset_d))
				out_listof_deleted_gosassetd->insertIfNotExists(elem.uid);
		}
		else
		{
			// vediamo se la risorsa e' stata modificata
			const u64 lastTimeMod = fs::fileGetLastTimeModified_UTC_niceu64(elem.abspath);
			if (lastTimeMod != elem.lastTimeModified)
			{
				elem.lastTimeModified = lastTimeMod;
				elem.status = eBuildStatus::MODIFIED;

				if (elem.uid.isAResourceOfType(eResType::gosasset_d))
				{
					out_listof_gosassetd_toRebuild->insertIfNotExists(elem.uid, elem.abspath);
				}
			}
		}

		if (eBuildStatus::UNCHANGED != elem.status)
		{
			priv_printResListElem(elem);

			// Cerco tutte le risorse che dipendono direttamente o indirettamente da questa e le elimino dal DB.
			// L'idea e' che se una risorsa e' stata modificata/eliminata, allora tutte quelle che ne necessitano devono comunque essere ricreate
			// perche' le dipendenze porebbero essere cambiate.
			// Elimino tutte queste risorse dal DB e, nel fare questo, mi segno quali risorse di tipo .gosasset_d sono coinvolte in moda da rebuildare
			// ed eventualmente ripopolare il DB
			// Le risorse di tipo gosasset_d le marco come da rebuildare
			UniqueUIDList uidList(localAllocator, 1024);
			dependency_get_requireBy_list(ctx, elem.uid, true, &uidList);

			uidList.forEach([&ctx, out_listof_gosassetd_toRebuild, out_listof_deleted_gosassetd, out_listOfPossibileConcreteAssetsToBeDeleted](u32 index, const UID uid)
			{
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
							out_listOfPossibileConcreteAssetsToBeDeleted->insertIfNotExists (uid_concrete_asset);
						}
					}
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


			//genero la lista di risorse da cui io dipendo. Se io sono stato modificato, allora forse anche
			//le risorse da cui dipendo potrebbe non essere piu' utilis
			dependency_get_dependecies_list(ctx, elem.uid, false, out_listOfPossibileResourceToBeDeleted);

			// delete dal DB
			uidList.forEach([&ctx](u32 index, const UID uid)
			{
				if (uid.isAResource())
					res_delete (ctx, uid);
				else if (uid.isVirtualAsset())
					virtasset_delete (ctx, uid);
				return true; 
			});
			
			res_delete(ctx, elem.uid);
		}
	}

	return true;
}

//******************************
bool Builder::priv_gosassetd_scan_folder(DBContext &ctx, const char *folder_path, HashedStringList *out_listof_gosassetd_toRebuild) const
{
	// Cerca tutte i file .gosasset_d nelle cartelle e sottocartelle del progetto
	bool ret = true;

	gos::FileFind ff;
	if (fs::findFirst(&ff, folder_path, "*.gosasset_d", eFileFindMode::only_file))
	{
		gos::StringList stringList(localAllocator, 4096);

		do
		{
			char absFilename[1024];
			sprintf_s(absFilename, sizeof(absFilename), "%s/%s", folder_path, fs::findGetFileName(ff));
			ret = priv_gosassetd_scan_folder_parse(ctx, absFilename, out_listof_gosassetd_toRebuild);
			if (!ret)
				break;
		} while (fs::findNext(ff));
		fs::findClose(ff);
	}

	// rifaccio il giro in cerca dei subfolder
	if (ret && fs::findFirst(&ff, folder_path, "*.*", eFileFindMode::only_folder))
	{
		do
		{
			char s[1024];
			sprintf_s(s, sizeof(s), "%s/%s", folder_path, fs::findGetFileName(ff));
			ret = priv_gosassetd_scan_folder(ctx, s, out_listof_gosassetd_toRebuild);
			if (!ret)
				break;
		} while (fs::findNext(ff));
		fs::findClose(ff);
	}

	return ret;
}

//******************************
bool Builder::priv_gosassetd_scan_folder_parse(DBContext &ctx, const char *absFilenameIN, HashedStringList *out_listof_gosassetd_toRebuild) const
{
	assert(fs::isPathAbsolute(absFilenameIN));

	// ho trovato un .gosasset_d in una della directory del progetto.
	// Verifico se era gia' nel DB o se e' nuovo
	UID uid;
	if (res_exists(ctx, eResType::gosasset_d, absFilenameIN, &uid))
		return true;

	// dato che non era gia' nel DB, deve per forza essere una risorsa nuova
	res_createUID(eResType::gosasset_d, absFilenameIN, &uid);
	out_listof_gosassetd_toRebuild->insertIfNotExists(uid, absFilenameIN);

	// info a video
	{
		sResListElem elem;
		elem.uid = uid;
		elem.lastTimeModified = fs::fileGetLastTimeModified_UTC_niceu64(absFilenameIN);
		sprintf_s(elem.abspath, sizeof(elem.abspath), "%s", absFilenameIN);
		elem.status = eBuildStatus::NEW;
		priv_printResListElem(elem);
	}

	return true;
}

//******************************
BuilderInterface *Builder::priv_findBuilderByClassName(const char *assetClassName) const
{
	for (u32 i = 0; i < NUM_MAX_BUILDERS; i++)
	{
		if (NULL == builderList[i])
			continue;

		if (0 == strcmp(asset2::enumToString(builderList[i]->getAssetType()), assetClassName))
			return builderList[i];
	}
	return NULL;
}

//******************************
void Builder::priv_fromDirectiveNameToAssetClassName(const char *directiveName, char *out_asseetClassName, u32 sizeof_out) const
{
	assert(directiveName[0] == '@');
	sprintf_s(out_asseetClassName, sizeof_out, "%s", &directiveName[1]);
	u32 i = 0;
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
bool Builder::priv_gosassetd_build(DBContext &ctx, bool bDoCreateAssetFile, const char *absFilename, UniqueUIDList *out_listOfBuiltAssets)
{
	// se esisto gia' nel DB, vuol dire che sono gia' stato rebuildato
	UID uid_of_iniFile;
	if (res_exists(ctx, eResType::gosasset_d, absFilename, &uid_of_iniFile))
	{
		logger->log("skipped because already built\n");
		return true;
	}
	else
	{
		// altrimenti mi addo al DB
		if (!res_insert(ctx, eResType::gosasset_d, absFilename, fs::fileGetLastTimeModified_UTC_niceu64(absFilename), &uid_of_iniFile))
			return false;
	}

	gos::IniFile ini;
	if (!ini.loadAndParse(absFilename))
	{
		logger->err("error parsing file %s\n", absFilename);
		return false;
	}

	UniqueStringList listof_knownRTname(localAllocator, 2048);
	UniqueUIDList listof_UID_of_known_ini_file(localAllocator, 32);
	listof_UID_of_known_ini_file.insertIfNotExists(uid_of_iniFile);

	// vado alla ricerca di eventuali #include
	gos::IniFileSection *section = ini.getRoot();
	for (u32 iSec = 0; iSec < section->getNSubsection(); iSec++)
	{
		// deve essere di tipo direttiva, altrimenti e' un errore
		gos::IniFileSection *sub = section->getSubsectionByIndex(iSec);
		const char *subName = sub->name.getBuffer();
		if (subName[0] != '@')
		{
			logger->err("line %d => invalid subsection, it must start with @\n", sub->getLineStarted());
			return false;
		}

		// verifico che tipo di asset sta descrivendo.
		char assetClass[64];
		priv_fromDirectiveNameToAssetClassName(subName, assetClass, sizeof(assetClass));

		// se non e' una @include o una @alias, skippo la sezione
		if (0 == strcmp(assetClass, "include"))
		{
			if (!priv_gosassetd_build_parseIncludeSection(ctx, bDoCreateAssetFile, absFilename, uid_of_iniFile, sub, listof_knownRTname, listof_UID_of_known_ini_file, out_listOfBuiltAssets))
				return false;
		}
		else if (0 == strcmp(assetClass, "alias"))
		{
			if (!priv_gosassetd_build_parseAliasSection(ctx, uid_of_iniFile, absFilename, sub))
				return false;
		}
	}

	// buildo tutte le sezioni
	u32 nextAnonymAssetName = 0;
	return priv_gosassetd_buildSection(ctx, bDoCreateAssetFile, nextAnonymAssetName, listof_knownRTname, listof_UID_of_known_ini_file, absFilename, uid_of_iniFile, ini.getRoot(), out_listOfBuiltAssets);
}

//******************************
bool Builder::priv_gosassetd_build_parseIncludeSection(DBContext &ctx, bool bDoCreateAssetFile, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sub, UniqueStringList &in_out__listof_knownRTname, UniqueUIDList &in_out__listof_UID_of_known_ini_file, UniqueUIDList *out_listOfBuiltAssets)
{
	// recupero il path dell'include
	char s[512];
	sub->getOrDefault("__value", "!", s, sizeof(s));
	if (s[0] == '!')
	{
		logger->log(eTextColor::red, "line %d, include does not state a path\n", sub->getLineStarted());
		return false;
	}

	char absIncludePath[512];
	if (!makeABSPathFromFilename(ctx, logger, in_out__listof_UID_of_known_ini_file, absFilename, s, absIncludePath, sizeof(absIncludePath)))
		return false;

	if (!fs::fileExists(absIncludePath))
	{
		logger->log(eTextColor::red, "line %d, included file '%s' does not exists\n", sub->getLineStarted(), absIncludePath);
		return false;
	}

	fs::extractFileExt(absIncludePath, s, sizeof(s));
	if (0 != strcmp(s, "gosasset_d"))
	{
		logger->log(eTextColor::red, "line %d, included file '%s' must be a 'gosasset_d' file\n", sub->getLineStarted(), absIncludePath);
		return false;
	}

	UID uid_of_included_ini;
	res_createUID(eResType::gosasset_d, absIncludePath, &uid_of_included_ini);
	if (!res_exists(ctx, eResType::gosasset_d, absIncludePath, NULL))
	{
		// non e' nel DB, vuol dire che devo prima buildarlo e poi posso proseguire con il build di me stesso
		logger->log("building included file %s\n", absIncludePath);
		logger->inc_indent();
		const bool ret = priv_gosassetd_build(ctx, bDoCreateAssetFile, absIncludePath, out_listOfBuiltAssets);
		logger->dec_indent();
		if (!ret)
			return false;
	}
	else
	{
		// e' gia' nel DB, vuol dire che non e' stata modificata/deletata oppure e' gia'
		// stat rebuildata
		logger->log("parsing included file %s\n", absIncludePath);
	}

	// io dipendo dal mio include
	dependency_add(ctx, uid_of_iniFile, uid_of_included_ini);

	// qui siamo sicuri che l'include e' stata buildata con successo.
	// Recupero un elenco di gosasset_d da cui <uid_of_included_ini> dipende (in sostanza, tutti gli include degli include degli include..
	in_out__listof_UID_of_known_ini_file.insertIfNotExists(uid_of_included_ini);
	dependency_get_dependecies_list(ctx, uid_of_included_ini, false, &in_out__listof_UID_of_known_ini_file, [](const UID childUID)
									{ return childUID.isAResourceOfType(eResType::gosasset_d); });

	// aggiungo tutti gli "rtname" definiti in tutti gli include degli include
	in_out__listof_UID_of_known_ini_file.forEach([&ctx, &in_out__listof_knownRTname](u32 index, const UID uid)
												 {
		assert (uid.isAResourceOfType(eResType::gosasset_d));
			

		char s[512];
		sprintf_s (s, sizeof(s), "SELECT rtname FROM " GOS_ASSET2__TABLE_VIRTUAL_ASSET " WHERE UID_ini=%" PRIu64 "", uid._uid);

		db::RST rst;
		if (db::query (ctx.db, s, &rst))
		{
			while (rst.fetchRow())
			{
				const char *rtname = rst.getVal(0);
				if (rtname[0] != '_' && rtname[1] != '_')
				{
					in_out__listof_knownRTname.add (rtname);
				}
			}			
		}
		return true; });

	return true;
}

//******************************
bool Builder::priv_gosassetd_build_parseAliasSection(DBContext &ctx, UID uid_of_iniFile, const char *absFilename, const gos::IniFileSection *sub)
{
	char alias[128];
	if (!sub->get("name", alias, sizeof(alias)))
	{
		logger->log(eTextColor::red, "line %d => invalid alias directive: param 'name' not found\n", sub->getLineStarted());
		return false;
	}

	char path[512];
	if (!sub->get("path", path, sizeof(path)))
	{
		logger->log(eTextColor::red, "line %d => invalid alias directive: param 'path' not found\n", sub->getLineStarted());
		return false;
	}

	char alias_abs_path[1024];
	fs::makeABSPathFromABSFilename(absFilename, path, alias_abs_path, sizeof(alias_abs_path));

	if (!alias_get_info(ctx, alias, NULL, 0, NULL))
	{
		if (!alias_insert(ctx, uid_of_iniFile, alias, alias_abs_path))
			return false;
		logger->log(eTextColor::darkYellow, "line %d => new alias '%s' => '%s'\n", sub->getLineStarted(), alias, alias_abs_path);
		return true;
	}

	logger->log(eTextColor::red, "line %d => alias '%s' was already defined (path: %s)\n", sub->getLineStarted(), alias, alias_abs_path);
	return false;
}

//******************************
bool Builder::priv_gosassetd_buildSection(DBContext &ctx, bool bDoCreateAssetFile, u32 &in_out_nextAnonymAssetName, UniqueStringList &in_out_listof_knownRTname, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFile, gos::IniFileSection *section, UniqueUIDList *out_listOfBuiltAssets)
{
	UniqueUIDList hashList1(localAllocator, 256);

	for (u32 iSec = 0; iSec < section->getNSubsection(); iSec++)
	{
		// deve essere di tipo direttiva, altrimenti e' un errore
		gos::IniFileSection *sub = section->getSubsectionByIndex(iSec);
		const char *subName = sub->name.getBuffer();
		if (subName[0] != '@')
		{
			logger->err("line %d => invalid subsection, it must start with @\n", sub->getLineStarted());
			return false;
		}

		// verifico che tipo di asset sta descrivendo.
		char assetClass[64];
		priv_fromDirectiveNameToAssetClassName(subName, assetClass, sizeof(assetClass));

		// se e' una @include/@alias, la skippo
		if (0 == strcmp(assetClass, "include") || 0 == strcmp(assetClass, "alias"))
			continue;

		// altrimenti verifico che ci sia un builder adeguato
		BuilderInterface *builder = priv_findBuilderByClassName(assetClass);
		if (NULL == builder)
		{
			logger->err("line %d => can't find a builder for asset of type '%s'\n", sub->getLineStarted(), assetClass);
			return false;
		}

		// se non ha un runtimeName per l'asset che descrive, gliene assegno uno d'ufficio
		char assetRuntimeName[128];
		if (!sub->exists("__value"))
		{
			sprintf_s(assetRuntimeName, sizeof(assetRuntimeName), "__%016" PRIX64 "_%06d", uid_of_iniFile._uid, in_out_nextAnonymAssetName++);
			sub->set("__value", assetRuntimeName);
		}
		else
			sub->get("__value", assetRuntimeName, sizeof(assetRuntimeName));

		// buildo
		logger->log("line %d, %s : %s\n", sub->getLineStarted(), assetClass, assetRuntimeName);
		logger->inc_indent();
		bool ret = false;
		while (1)
		{
			// se e' il nome di una sezione con un runtime-name ma nessun parametro, vuol dire che e' una direttiva
			// gia' risolta che punta ad un runtime-name ben specifico.
			// In sostanza, e' una sottodirettiva di una direttiva di livello superiore (es: @vtx_shader all'interno di @pipe)
			// Il rt-name deve essermi gia' noto
			if (1 == sub->getNIdentifier() && assetRuntimeName[0] != '_' && assetRuntimeName[1] != '_')
			{
				if (!in_out_listof_knownRTname.exists(assetRuntimeName))
				{
					ret = false;
					logger->log(eTextColor::red, "rt-name %s is unknown\n", assetRuntimeName);
				}
				else
				{
					ret = true;
					logger->log("skip because is an inline-directive\n");
				}
				break;
			}

			// asset con depth > 1 possono avere delle subsection inline, che devo risolvere prima
			// di poter buildare l'asset
			if (sub->getNSubsection())
			{
				if (!priv_gosassetd_buildSection(ctx, bDoCreateAssetFile, in_out_nextAnonymAssetName, in_out_listof_knownRTname, listof_UID_of_known_ini_file, absFilename, uid_of_iniFile, sub, out_listOfBuiltAssets))
					break;
			}


			//invoco il builder per creare fisicamente gli asset
			bool bBuildSuccess = false;
			builder->setLogger(logger);
			if (builder->build_begin(ctx, listof_UID_of_known_ini_file, absFilename, uid_of_iniFile, sub))
			{
				bool bContinue = true;
				while(bContinue)
				{
					sBuildResult result;
					bBuildSuccess = builder->build_exe (ctx, bDoCreateAssetFile, &bContinue, &result);
					if (false == bBuildSuccess)
						break;

					// report a video del risultato della build
					eTextColor color = eTextColor::green;
					if (eBuildResult::was_already_built == result.result)
						color = eTextColor::darkBlue;
					logger->log(color, "[%-17s] %016" PRIX64 " [%016" PRIX64 "]\n", asset2::enumToString(result.result), result.uid_virtual_asset._uid, result.uid_concrete_asset._uid);


					// calcolo e scrivo le dipendenze runtime di questo asset
					// Per "dipendenze runtime" intendo una lista di altri asset (e non risorse) dai quali questo asset dipende
					if (eBuildResult::just_built == result.result)
					{
						out_listOfBuiltAssets->insertIfNotExists(result.uid_concrete_asset);

						dependency_get_dependecies_list(ctx, result.uid_concrete_asset, true, &hashList1);
						auto list = hashList1._queryList();
						for (u32 i = 0; i < list->getNElem(); i++)
						{
							const UID childUID = list->queryElem(i);
							if (childUID.isAnAsset())
								dependencyRT_add(ctx, result.uid_concrete_asset, childUID);
						}
					}
				}
				builder->build_end();
			}
			if (!bBuildSuccess)
				break;

			// aggiungo rt-name alla lista dei nomi noti da questo file
			in_out_listof_knownRTname.add(assetRuntimeName);

			// fine while (1)
			ret = true;
			break;
		}
		logger->dec_indent();

		if (!ret)
			return false;
	}

	return true;
}

//******************************************
bool Builder::makeABSPathFromFilename(DBContext &ctx, gos::Logger *logger, const UniqueUIDList &listof_UID_of_known_ini_file, const char *origin_absFilename, const char *rel_or_abs_path, char *out, u32 sizeof_out)
{
	assert(NULL != logger);
	assert(ctx.isValid());

	if (rel_or_abs_path[0] == '<')
	{
		// inizia con una alias definito in qualche include file
		char alias[128];
		alias[0] = 0x00;

		u32 i = 0;
		while (rel_or_abs_path[i++] != 0x00)
		{
			if (rel_or_abs_path[i] == '>')
			{
				memcpy(alias, &rel_or_abs_path[1], i - 1);
				alias[i - 1] = 0x00;
				break;
			}
		}
		if (0x00 == alias[0])
		{
			logger->log(eTextColor::red, "invalid alias: it starts with '<' but does not end with '>' [%s]\n", rel_or_abs_path);
			return false;
		}

		// cerco l'alias nel DB
		UID uid_of_inifile_where_alias_is_defined;
		char alias_resolved[1024];
		if (!alias_get_info(ctx, alias, alias_resolved, sizeof(alias_resolved), &uid_of_inifile_where_alias_is_defined))
		{
			logger->log(eTextColor::red, "invalid alias: [%s] not found in DB\n", alias);
			return false;
		}

		// ho trovato l'alias ma l'UID dell'ini file in cui l'alias e' stato definito deve essere uno degli UID che io conosco (ovvero che ho incluso)
		if (!listof_UID_of_known_ini_file.exists(uid_of_inifile_where_alias_is_defined))
		{
			logger->log(eTextColor::red, "alias [%s] is not defined in this module\n", alias);
			return false;
		}

		char s[1024];
		sprintf_s(s, sizeof(s), "%s%s", alias_resolved, &rel_or_abs_path[i + 1]);
		fs::makeABSPathFromABSFilename(origin_absFilename, s, out, sizeof_out);
		return true;
	}
	else
	{
		fs::makeABSPathFromABSFilename(origin_absFilename, rel_or_abs_path, out, sizeof_out);
		return true;
	}
}
