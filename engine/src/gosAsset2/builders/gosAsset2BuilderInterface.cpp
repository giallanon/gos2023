#include "gosAsset2BuilderInterface.h"
#include "string/gosStringIncludeDetector.h"

using namespace gos;
using namespace gos::asset2;


//******************************************
void BuilderInterface::prot_makeABSPath (const char *absFilename, const char *path, char *out, u32 sizeof_out) const
{
    assert (fs::isPathAbsolute(absFilename));

    //i path degli include possono essere relativi
    char s[1024];
    if (fs::isPathAbsolute(path))
        sprintf_s (s, sizeof(s), "%s", path);
    else
    {
        fs::extractFilePathWithSlash (absFilename, s, sizeof(s));
        strcat_s (s, sizeof(s), path);
    }    

    fs::pathSanitizeInPlace(s);
    const u32 len = string::ansi::lengthInBytes(s);

    assert (len < sizeof_out);
    memset (out, 0, sizeof_out);
    memcpy (out, s, len);
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
bool BuilderInterface::prot_needResolvedSubsection (DBContext &ctx, const gos::IniFileSection *sec, eAssetType assType, UID *out_uid) const
{
    assert (NULL != sec);
    assert (NULL != out_uid);

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

            if (!asset2::rtname_exists (ctx, s, out_uid))
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
        prot_makeABSPath (absFilenameIN, s, absIncludeFilename, sizeof(absIncludeFilename));
		out->add(absIncludeFilename);
	}

	GOSFREE_SCRAP (buffer);
	return ret;
}