#include "gosAssetHub.h"
#include "gosAsset.h"


using namespace gos;
using namespace gos::asset;


//***************************************
Hub::Hub()
{
    localAllocator = GOSNEW(gos::getSysHeapAllocator(), LocalAllocator)("AssetHub");
    localAllocator->setup (16* 1024 * 1024);

    knownAssetsList.setup (localAllocator, 0xffff);
}

//***************************************
Hub::~Hub()
{
    gos::Allocator *a = localAllocator;
    knownAssetsList.forEach ([a] (u64 uid, void *pt) 
    {
        GOSFREE(a, pt);
        return true;
    });
    knownAssetsList.unsetup();



    GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
    localAllocator = NULL;
}

//***************************************
bool Hub::setup (const char *baseFolder, gos::GPU *gpu)
{
    return loader.setup (baseFolder, gpu);
}

//***************************************
 bool Hub::getHandle (const char *runtimeName, Handle *out)
 {
    asset::UID uid;
    if (!loader.runtimeNameToUID(runtimeName, &uid))
    {
        gos::logger::err ("asset::Hub::getHandle() => invalid runtineName '%s'\n", runtimeName);
        return false;
    }


    HashList::Position pos;
    void *pt = NULL;
    if (knownAssetsList.findWithPos (uid._uid, &pt, &pos))
    {
        //l'asset esiste gia in memoria
        out->_pt = pt;
        return true;
    }

    //devo creare l'asset
    //Non lo carico per il momento, ma creo lo spazio in memoria necessario
    asset::LoaderInterface *l = loader.getLoader( uid.getAssetType());
    if (NULL == l)
    {
        gos::logger::err ("asset::Hub::getHandle() => can't find a loader for resource %s\n", asset::enumToString(uid.getAssetType()));
        return false;
    }    

    const u32 sizeof_data = sizeof(sHeader) + l->getSizeOfData();
    void *buffer = GOSALLOCT(u8*, localAllocator, sizeof_data);
    knownAssetsList.insertInPosition (pos, buffer);
    out->_pt = buffer;


    sHeader *header = static_cast<sHeader*>(buffer);
    memset (header, 0, sizeof(sHeader));
    header->status = eStatus::notLoaded;
    header->uid = uid;

    

    return true;
}

//***************************************
 Hub::eStatus Hub::priv_getAsset (const Handle &h, const void **out_assetData) const
 {
    const sHeader *header = static_cast<const sHeader*>(h._pt);
    if (eStatus::ready == header->status)
    {
        *out_assetData = &header[sizeof(sHeader)];
    }
    else
    {
        *out_assetData = NULL;
    }

    return header->status;
 }