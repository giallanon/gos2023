#include "gosAsset2BuilderInterface.h"
#include "string/gosStringIncludeDetector.h"
#include "../gosAsset2Builder.h"

using namespace gos;
using namespace gos::asset2;




//******************************************
bool BuilderInterface::prot_isOneOfThis (const char *paramName, ...) const
{
    if (paramName[0] == '_' && paramName[1] == '_')
        return true;
        
    va_list args;
    va_start (args, paramName);
    
    bool ret = false;
    while (1)
    {
        const char *s = va_arg(args, const char*);
        if (NULL == s)
            break;
        if (strcmp (paramName, s) == 0)
        {
            ret = true;
            break;
        }
    }

    va_end (args);
    return ret;
}

//******************************************
bool BuilderInterface::prot_needResolvedSubsection (DBContext &ctx, const gos::IniFileSection *sec, eAssetType assType, UID *out__assetUID) const
{
    assert (NULL != sec);
    assert (NULL != out__assetUID);

    char s[128];
    const char *assTypeName = asset2::enumToString (assType);
    sprintf_s (s, sizeof(s), "@%s@", assTypeName);
    const u32 nameLen = static_cast<u32> (strlen(s));

    for (u32 i=0; i<sec->getNSubsection(); i++)
    {
        const gos::IniFileSection *sub = sec->getSubsectionByIndex (i);
        if (sub->name.isEqualToWithLen (s, nameLen, false))
        {
            sub->getOrDefault ("__value", "!", s, sizeof(s));
            if (s[0] == '!')
                return false;

            if (!asset2::asset_rtname_exists (ctx, s, out__assetUID))
            {
                logger->log (eTextColor::red, "invalid rtname: %s\n", s);
                return false;
            }

            return true;
        }
    }

    return false;

}

/******************************************
 * In caso di risorse che includo a loro volta altre risorse (al momento solo gli shader_txt), all'uscita di questa
 * fn l'array <list_of_nested_resources> contiene un elenco delle risorse incluse
 */
bool BuilderInterface::prot_needResource (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, eResType resTypeIN, const char *absFilenameIN, ResourceDep *out)
{
	num_nested_resources = 0;
	return priv_do_needResource (ctx, listof_UID_of_known_ini_file, resTypeIN, absFilenameIN, out);
}

bool BuilderInterface::priv_do_needResource (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, eResType resTypeIN, const char *absFilenameIN, ResourceDep *out)
{
    assert (NULL != absFilenameIN);
    assert (NULL != out);
    assert (fs::isPathAbsolute(absFilenameIN));

    if (!asset2::res_exists (ctx, resTypeIN, absFilenameIN, &out->uid, &out->lastTimeMod))
	{
		//<absFilenameIN> non esiste nel DB, la devo aggiungere
		if (!fs::fileExists(absFilenameIN))
		{
			logger->log (eTextColor::red, "can't open file %s\n", absFilenameIN);
			return false;
		}

		out->lastTimeMod = fs::fileGetLastTimeModified_UTC_niceu64(absFilenameIN);
		if (!asset2::res_insert (ctx, resTypeIN, absFilenameIN, out->lastTimeMod, &out->uid))
		{
			logger->log (eTextColor::red, "error inserting resource %s\n", absFilenameIN);
			return false;
		}
	}
	
    //le risorse shader possono avere delle include.
    //Devo aggiungere la dipendenza di this dalle sue include
    if (eResType::shader_txt == resTypeIN)
    {
        gos::StringList includeList(gos::getScrapAllocator(), 1024);
        if (!priv_extractAllInludePaths(ctx, listof_UID_of_known_ini_file, absFilenameIN, &includeList))
        {
            logger->log (eTextColor::red, "error extracting include-file from resource %s\n", absFilenameIN);
            return false;
        }

        u32 iter;
        const char *absIncludePath;
        includeList.toStart(&iter);
        while (NULL != (absIncludePath = includeList.next(&iter)))
        {
			ResourceDep *nested_res = &list_of_nested_resources[num_nested_resources++];
            if (priv_do_needResource (ctx, listof_UID_of_known_ini_file, eResType::shader_txt, absIncludePath, nested_res))
            {
                if (!dependency_exists (ctx, out->uid, nested_res->uid))
                    dependency_add (ctx, out->uid, nested_res->uid);
            }

			if (num_nested_resources >= NUM_MAX_NESTED_RESOURCES)
			{
				logger->log (eTextColor::red, "error, too many nested resources while working with %s\n", absFilenameIN);
				return false;
			}
        }
    }


    return true;
}

//****************************** 
bool BuilderInterface::priv_extractAllInludePaths (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilenameIN, gos::StringList *out) const
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
        if (!asset2::Builder::makeABSPathFromFilename (ctx, logger, listof_UID_of_known_ini_file, absFilenameIN, s, absIncludeFilename, sizeof(absIncludeFilename)))
            return false;
		out->add(absIncludeFilename);
	}

	GOSFREE_SCRAP (buffer);
	return ret;
}

/****************************** 
 * <params> e' usato per determinare la uid-signature a partire dai parametri specificati in un gosasset_d
 */
bool BuilderInterface::prot_setupAsset (DBContext &ctx, const void *params, u32 sizeof_params, UID uid_of_iniFile, const gos::IniFileSection *sec, sBuildResult *out_result) const
{
    //recupero il rtname dell'asset
	char rtname[128];
    memset (rtname, 0, sizeof(rtname));
    sec->get("__value", rtname, sizeof(rtname));

	return prot_setupAsset_ex (ctx, getSignatureType(), params, sizeof_params, rtname, uid_of_iniFile, sec->getLineStarted(), out_result);
}


/****************************** 
 * <params> e' usato per determinare la uid-signature a partire dai parametri specificati in un gosasset_d
 * <uid_of_iniFile> e' UID dell'inifile nel quale questo asset e' dichiarato
 */
bool BuilderInterface::prot_setupAsset_ex (DBContext &ctx, eAssetType assetType, const void *params, u32 sizeof_params, const char *rtname, 
	UID uid_of_iniFile, 
	u32 asset__declared_on_lineNum, 
	sBuildResult *out_result) const
{
    //calcolo uid-signature
    if (!signature_createUID (assetType, params, sizeof_params, &out_result->signatureUID))
    {
        gos::logger::err ("error generating signatureUID\n");
        return false;
    }


    //inserisco il virtual asset nel DB
    if (!asset_insert (ctx, assetType, rtname, uid_of_iniFile, asset__declared_on_lineNum, out_result->signatureUID, &out_result->assetUID))
    {
        logger->log (eTextColor::red, "error inserting UID of virtual asset\n");
        return false;
    }    

    //vediamo se la signature esiste gia' nel DB
    if (asset2::signature_exists (ctx, out_result->signatureUID))
    {
        out_result->result = eBuildResult::was_already_built;
    }
    else
    {
        //non esisteva nel DB, ottimo, lo aggiungo e poi lo buildo
        out_result->result = eBuildResult::just_built;
        if (!signature_insert (ctx, out_result->signatureUID))
        {
            logger->log (eTextColor::red, "error inserting signatureUID in DB\n");
            return false;
        }        
    }

    //aggiungo le dipendenze di assetUID verso l'inifile e il concrete asset
    if (!dependency_add (ctx, out_result->assetUID, uid_of_iniFile)) return false;
    if (!dependency_add (ctx, out_result->assetUID, out_result->signatureUID)) return false;

    return true;
}