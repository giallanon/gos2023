#include "gosAssetBuilderInterface.h"
#include "../gosAsset.h"


using namespace gos;
using namespace gos::asset;


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
bool BuilderInterface::prot_needResolvedSubsection (Context &ctx, const gos::IniFileSection *sec, eAssetType assType, asset::UID *out_uid) const
{
    assert (NULL != sec);
    assert (NULL != out_uid);

    char s[128];
    const char *assTypeName = asset::enumToString (assType);
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

            if (!asset::rtname_exists (ctx, s, out_uid))
            {
                logger::err ("invalid rtname: %s\n", s);
                return false;
            }

            return true;
        }
    }

    return false;

}

//******************************************
bool BuilderInterface::prot_needResource (Context &ctx, eResType resType, const char *resName, asset::UID *out_uid) const
{
    assert (NULL != resName);
    assert (NULL != out_uid);

    return asset::res_exists (ctx, resType, resName, out_uid);
}