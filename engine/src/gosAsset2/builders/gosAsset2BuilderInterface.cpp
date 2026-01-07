#include "gosAsset2BuilderInterface.h"
#include "string/gosStringIncludeDetector.h"
#include "../gosAsset2Builder.h"

using namespace gos;
using namespace gos::asset2;



//******************************************
bool BuilderInterface::prot_makeABSPathFromFilename (const char *origin_absFilename, const char *rel_or_abs_path, char *out, u32 sizeof_out) const
{
    return theBuilder->internal__makeABSPathFromFilename (origin_absFilename, rel_or_abs_path, out, sizeof_out);

}

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
bool BuilderInterface::prot_needResolvedSubsection (DBContext &ctx, const gos::IniFileSection *sec, eAssetType assType, UID *out__virtual_uid) const
{
    assert (NULL != sec);
    assert (NULL != out__virtual_uid);

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

            if (!asset2::virtasset_rtname_exists (ctx, s, out__virtual_uid))
            {
                logger->log (eTextColor::red, "invalid rtname: %s\n", s);
                return false;
            }

            return true;
        }
    }

    return false;

}

//******************************************
bool BuilderInterface::prot_needResource (DBContext &ctx, eResType resTypeIN, const char *absFilenameIN, UID *out_uid) const
{
    assert (NULL != absFilenameIN);
    assert (NULL != out_uid);
    assert (fs::isPathAbsolute(absFilenameIN));

    if (asset2::res_exists (ctx, resTypeIN, absFilenameIN, out_uid))
        return true;

    //<absFilenameIN> non esiste nel DB, la devo aggiungere
    if (!fs::fileExists(absFilenameIN))
    {
        logger->log (eTextColor::red, "can't open file %s\n", absFilenameIN);
        return false;
    }

    const u64 lastTimeMod = fs::fileGetLastTimeModified_UTC_niceu64(absFilenameIN);
    if (!asset2::res_insert (ctx, resTypeIN, absFilenameIN, lastTimeMod, out_uid))
    {
        logger->log (eTextColor::red, "error inserting resource %s\n", absFilenameIN);
        return false;
    }

    //le risorse shader possono avere delle include.
    //DEvo aggiungere la dipendenza di this dalle sue include
    if (eResType::shader_txt == resTypeIN)
    {
        gos::StringList includeList(gos::getScrapAllocator(), 1024);
        if (!priv_extractAllInludePaths(absFilenameIN, &includeList))
        {
            logger->log (eTextColor::red, "error extracting include-file from resource %s\n", absFilenameIN);
            return false;
        }

        u32 iter;
        const char *absIncludePath;
        includeList.toStart(&iter);
        while (NULL != (absIncludePath = includeList.next(&iter)))
        {
            UID shaderUID;
            if (prot_needResource (ctx, eResType::shader_txt, absIncludePath, &shaderUID))
            {
                if (!dependency_exists (ctx, *out_uid, shaderUID))
                    dependency_add (ctx, *out_uid, shaderUID);
            }
        }
    }


    return true;
}

//****************************** 
bool BuilderInterface::priv_extractAllInludePaths (const char *absFilenameIN, gos::StringList *out) const
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
        prot_makeABSPathFromFilename (absFilenameIN, s, absIncludeFilename, sizeof(absIncludeFilename));
		out->add(absIncludeFilename);
	}

	GOSFREE_SCRAP (buffer);
	return ret;
}

//****************************** 
bool BuilderInterface::prot_setupVirtualAsset (DBContext &ctx, const void *params, u32 sizeof_params, UID uid_of_iniFile, const gos::IniFileSection *sec, sBuildResult *out_result) const
{
    //calcolo assetUID
    if (!asset_createUID (getAssetType(), params, sizeof_params, &out_result->uid_concrete_asset))
    {
        gos::logger::err ("error generating UID of concrete asset\n");
        return false;
    }


    //inserisco il virtual asset nel DB
    char rtname[128];
    memset (rtname, 0, sizeof(rtname));
    sec->get("__value", rtname, sizeof(rtname));
    if (!virtasset_insert (ctx, getAssetType(), rtname, uid_of_iniFile, sec->getLineStarted(), out_result->uid_concrete_asset, &out_result->uid_virtual_asset))
    {
        logger->log (eTextColor::red, "error inserting UID of virtual asset\n");
        return false;
    }    

    //vediamo se il concrete-asset esiste gia' nel DB
    if (asset2::asset_exists (ctx, out_result->uid_concrete_asset))
    {
        out_result->result = eBuildResult::was_already_built;
    }
    else
    {
        //non esisteva nel DB, ottimo, lo aggiungo e poi lo buildo
        out_result->result = eBuildResult::just_built;
        if (!asset_insert (ctx, out_result->uid_concrete_asset))
        {
            logger->log (eTextColor::red, "error inserting asset in DB\n");
            return false;
        }        
    }

    //aggiungo le dipendenze di virtual-asset ve l'inifile e il concrete asset
    if (!dependency_add (ctx, out_result->uid_virtual_asset, uid_of_iniFile)) return false;
    if (!dependency_add (ctx, out_result->uid_virtual_asset, out_result->uid_concrete_asset)) return false;

    return true;
}