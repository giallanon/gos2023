#include "gosAsset2Builder.h"
#include "gosAsset2.h"
#include "gos.h"
#include "string/gosStringIncludeDetector.h"

using namespace gos;
using namespace gos::asset2;


//****************************** 
Builder::Builder()
{
	localAllocator = gos::getSysHeapAllocator();
}

//****************************** 
Builder::~Builder()
{}

//****************************** 
bool Builder::rebuildAll (const char *baseFolder)
{
	char s[1024];
	sprintf_s (s, sizeof(s), "%s/assets2.sqlite3", baseFolder);
	fs::fileDelete(s);

	return build (baseFolder);
}

//****************************** 
bool Builder::build (const char *baseFolder)
{
	bool ret = true;

	DBContext	ctx;
	if (!asset2::dbcontext_open (baseFolder, &ctx))
		return false;


	//STEP 1
	// recupero un elenco di .gosasset_d che sono nuovi, modificati o deleted
	ResList listof_gosAssetd(localAllocator, 256);
	{
		priv_gosassetd_scan_DB (ctx, &listof_gosAssetd);
		ret = priv_gosassetd_scan_folder (ctx, baseFolder, &listof_gosAssetd);
	}

	if (ret)
	{
	}

	//fine
	asset2::dbcontext_close (ctx);
	return ret;
}

//****************************** 
void Builder::priv_gosassetd_scan_DB (DBContext &ctx, ResList *out_list) const
{
	char s[1024];
    sprintf_s (s, sizeof(s), "SELECT UID,lastTimeMod,abspath FROM " GOS_ASSET2__TABLE_RES " WHERE type=%d ORDER BY UID", (u8)eResType::gosasset_d);

	db::RST rst;
	if (db::query (ctx.db, s, &rst))
	{
		while (rst.fetchRow())
		{
			sResListElem elem;
			elem.uid = rst.getValAsU64(0);;
			elem.lastTimeModified = rst.getValAsU64(1);
			sprintf_s (elem.abspath, sizeof(elem.abspath), "%s", rst.getVal(2));
			elem.resType = eResType::gosasset_d;
			elem.status = eBuildStatus::UNCHANGED;

			if (!fs::fileExists (elem.abspath))
				elem.status = eBuildStatus::DELETED;
			else
			{
				const u64 lastTimeMod = fs::fileGetLastTimeModified_UTC_niceu64(elem.abspath);
				if (lastTimeMod != elem.lastTimeModified)
				{
					elem.lastTimeModified = lastTimeMod;
					elem.status = eBuildStatus::MODIFIED;
				}
			}

			out_list->append(elem);
		}
	}

}

//****************************** 
bool Builder::priv_gosassetd_scan_folder (DBContext &ctx, const char *folder_path, ResList *out_list) const
{
	bool ret = true;

	gos::FileFind ff;
	if (fs::findFirst (&ff, folder_path, "*.gosasset_d"))
	{
		//al primo giro salto le directory
		do
		{
			if (fs::findIsDirectory(ff))
				continue;

			char s[1024];
			sprintf_s (s, sizeof(s), "%s/%s", folder_path, fs::findGetFileName(ff));
			ret = priv_gosassetd_scan_folder_parse(ctx, s, out_list);
			if (!ret)
				break;
		} while (fs::findNext(ff));
		fs::findClose (ff);

		//rifaccio il giro in cerca dei subfolder
		if (ret)
		{
			fs::findFirst (&ff, folder_path, "*.gosasset_d");
			do
			{
				if (!fs::findIsDirectory(ff))
					continue;

				const char *fname = fs::findGetFileName(ff);
				if (fname[0] == '.')
					continue;

				char s[1024];
				sprintf_s (s, sizeof(s), "%s/%s", folder_path, fname);
				ret = priv_gosassetd_scan_folder(ctx, s, out_list);
				if (!ret)
					break;
			} while (fs::findNext(ff));
			fs::findClose (ff);
		}
	}

	return ret;
}

//****************************** 
bool Builder::priv_gosassetd_scan_folder_parse (DBContext &ctx, const char *absFilenameIN, ResList *out_list) const
{
	bool bAlreadyInDB = false;
	out_list->forEach([absFilenameIN, &bAlreadyInDB](u32 index, const sResListElem &elem) {
		if (strcmp(absFilenameIN, elem.abspath) == 0)
		{
			//ho trovato <absFilenameIN> nella lista, vuol dire che il file era gia' nel DB.
			//Deve per forza quindi trovarsi in stato DELETED | MODIFIED
			assert (eBuildStatus::MODIFIED == elem.status || eBuildStatus::DELETED == elem.status);
			bAlreadyInDB = true;
			return false;
		}
		return true;
	});

	if (bAlreadyInDB)
		return true;

	//dato che non era nel DB, deve essere una risorsa nuova
	sResListElem elem;
	elem.status = eBuildStatus::NEW;
	elem.resType = eResType::gosasset_d;
	elem.lastTimeModified = fs::fileGetLastTimeModified_UTC_niceu64(absFilenameIN);
	sprintf_s (elem.abspath, sizeof(elem.abspath), "%s", absFilenameIN);
	res_createUID (eResType::gosasset_d, absFilenameIN, &elem.uid);
	
	out_list->append(elem);	
	return true;
}


