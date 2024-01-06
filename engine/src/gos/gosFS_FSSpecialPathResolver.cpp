#include "gosFS_FSSpecialPathResolver.h"
#include "gos.h"
#include "gosString.h"

using namespace gos;


//*********************************************
void fs::SpecialPathResolver::setup (gos::Allocator *allocatorIN)
{
    allocator = allocatorIN;
    listString.setup (allocator, 32);
}

//*********************************************
void fs::SpecialPathResolver::unsetup()
{
    if (NULL == allocator)
        return;

    const u32 n= listString.getNElem();
    for (u32 i=0; i<n; i++)
    {
        GOSFREE(allocator, listString[i].alias);    
        GOSFREE(allocator, listString[i].realPathNoSlash);
    }
    listString.unsetup();
    
    allocator = NULL;
}


//*********************************************
bool fs::SpecialPathResolver::addAlias (const char *alias, const u8 *realPathNoSlash) 
{
    assert (alias[0] == '@');

    const sAlias *s = priv_findAlias(reinterpret_cast<const u8*>(alias));
    if (NULL != s)
    {
        gos::logger::err ("fs::addAlias(%s,%s) => alias already exists and point to %s\n", alias, realPathNoSlash, s->realPathNoSlash);
        return false;
    }


    const u32 n = listString.getNElem();
    listString[n].alias = string::utf8::allocStr (gos::getSysHeapAllocator(), &alias[1]);
    listString[n].realPathNoSlash = string::utf8::allocStr (gos::getSysHeapAllocator(), realPathNoSlash);

    fs::pathSanitizeInPlace (listString[n].realPathNoSlash);
    return true;
}

//*********************************************
const fs::SpecialPathResolver::sAlias* fs::SpecialPathResolver::priv_findAlias (const u8 *alias) const
{
    const u32 n= listString.getNElem();
    for (u32 i=0; i<n; i++)
    {
        if (string::utf8::areEqual (listString(i).alias, alias, true))
            return &listString(i);
    }

    return NULL;
}

//*********************************************
void fs::SpecialPathResolver::resolve (const u8 *path, u8 *out, u32 sizeof_out) const
{
    assert (out);
    assert(sizeof_out);

    if (path[0] == '@')
    {
        priv_resolve (path, out, sizeof_out);
        return;
    }

#ifdef GOS_PLATFORM__LINUX
    if (path[0] != '/')
    {
        //se non inzia con '/' vuol dire che e' un path relativo
        gos::string::utf8::spf (out, sizeof_out, "%s/%s", gos::getAppPathNoSlash(), path);
        return;
    }
#endif
#ifdef GOS_PLATFORM__WINDOWS
    if (path[0] != ':')
    {
        gos::string::utf8::spf (out, sizeof_out, "%s/%s", gos::getAppPathNoSlash(), path);
        return;
    }
#endif


    //era un path normale
    gos::string::utf8::spf (out, sizeof_out, "%s", path);
}

//*********************************************
bool fs::SpecialPathResolver::priv_resolve (const u8 *path, u8 *out, u32 sizeof_out)  const
{
    string::utf8::Iter src;
    string::utf8::Iter result;

    UTF8Char closingChar('/');
    src.setup (&path[1]);
    if (string::utf8::extractValue (src, &result, &closingChar, 1))
    {
        result.copyAllStr (out, sizeof_out);
        src.advanceOneChar();

        const sAlias *s = priv_findAlias(out);
        if (NULL != s)
        {
            string::utf8::spf (out, sizeof_out, "%s/%s", s->realPathNoSlash, src.getPointerToCurrentPosition());
            return true;
        }
    }

    DBGBREAK;
    return false;
}


