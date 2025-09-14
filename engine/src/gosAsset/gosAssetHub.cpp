#include "gosAssetHub.h"
#include "gosAsset.h"


using namespace gos;
using namespace gos::asset;


//***************************************
Hub::Hub()
{
    localAllocator = GOSNEW(gos::getSysHeapAllocator(), LocalAllocator)("AssetHub");
    localAllocator->setup (16* 1024 * 1024);
    logger = gos::logger::getSystemLogger();
    lastTimeUpdateWasCalled_msec = 0;
    knownAssetsList.setup (localAllocator, 0xffff);
    fastUIDList.setup (localAllocator, 64);
}

//***************************************
void Hub::priv_free ()
{
    //chiedo al thread di morire e aspetto che termini
    thread::pushMsg (msgq_1W, THREADMSG_1_DIE, 0);
    thread::waitEnd (hThreadLoader);
    thread::deleteMsgQ (msgq_1R, msgq_1W);
    thread::deleteMsgQ (msgq_2R, msgq_2W);

    //free vari
    gos::Allocator *a = localAllocator;
    knownAssetsList.forEach ([a] (asset::UID uid, void *pt) 
    {
        GOSFREE(a, pt);
        return true;
    });
    
    knownAssetsList.unsetup();
    fastUIDList.unsetup();

    
    GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
    localAllocator = NULL;
}

//***************************************
bool Hub::setup (const char *baseFolderIN, gos::GPU *gpuIN)
{
    if (!loader.setup (baseFolderIN, gpuIN, this))
    {
        logger->err ("asset::Hub::setup() => error initializing loader in '%s'\n", baseFolderIN);
        return false;
    }

    //creo 2 code, una che uso per mandare msg da this al thread e una per mandare
    //msg dal thread a this
    thread::createMsgQ (&msgq_1R, &msgq_1W);
    thread::createMsgQ (&msgq_2R, &msgq_2W);


    //creo il thread principale
    sThreadParams params;
    params.msgqR = msgq_1R;
    params.msgqW = msgq_2W;
    params.baseFolder = baseFolderIN;
    params.loader = &loader;
    params.logger = logger;
    thread::eventCreate (&params.hEvent_started);

    eThreadError err = thread::create (&hThreadLoader, Hub::ThreadFN_main, &params);
    if (err != eThreadError::none)
    {
        logger->err ("asset::Hub::setup() => error creating thread: errcode=%d\n", static_cast<int>(err));
        return false;
    }

    if (!thread::eventWait (params.hEvent_started, 20000))
    {
        logger->err ("asset::Hub::setup() => error waiting for thread to start\n");
        return false;
    }

    thread::eventDestroy (params.hEvent_started);

    return true;
}

//***************************************
void Hub::update (u64 timenow_msec)
{
    static constexpr u8 NUM_MAX_MESSAGES = 16;
    thread::sMsg msgList[NUM_MAX_MESSAGES];

    const u64 timeout_msec = timenow_msec + 15;
    u32 nMsg;
    while (0 != (nMsg = thread::popMultipleMsg(msgq_2R, msgList, NUM_MAX_MESSAGES)))
    {
        for (u32 i=0; i<nMsg; i++)
        {
            switch (msgList[i].what)
            {
            default:
                DBGBREAK;
                break;

            case THREADMSG_2_CHANGE_STATUS:
                //il thread mi segnala che devo cambiare stato ad una risorsa
                {
                    void *pt = (void*)msgList[i].paramU64;
                    sHeader *header = static_cast<sHeader*>(pt);
                    header->external_status = static_cast<eStatusPublic> (msgList[i].bufferSize);
                }
                break;
            }

            thread::deleteMsg (msgList[i]);
        }

        lastTimeUpdateWasCalled_msec = gos::getTimeSinceStart_msec();
        if (lastTimeUpdateWasCalled_msec >= timeout_msec)
            break;
    }
}

/***************************************
 * ritorna true se l'asset esisteva gia'
 */
bool Hub::priv_findOrAddAsset (const asset::UID &uid, void **out_pt)
{
    //se l'asset esiste gia'...
    HashList::Position pos;
    void *pt = NULL;
    if (knownAssetsList.findWithPos (uid, &pt, &pos))
    {
        *out_pt = pt;
        return true;
    }
    
    //..altrimenti devo creare il spazio in memoria.
    asset::LoaderInterface *l = loader.getLoader( uid.getAssetType());
    if (NULL == l)
    {
        gos::logger::err ("asset::Hub::priv_findOrAddAsset() => can't find a loader for resource %s\n", asset::enumToString(uid.getAssetType()));
        *out_pt = NULL;
        return false;
    }    

    const u32 sizeof_data = sizeof(sHeader) + l->getSizeOfData();
    pt = GOSALLOCT(u8*, localAllocator, sizeof_data);
    knownAssetsList.insertInPosition (pos, pt);
    *out_pt = pt;


    sHeader *header = static_cast<sHeader*>(pt);
    memset (header, 0, sizeof(sHeader));
    header->external_status = eStatusPublic::notLoaded;
    header->internal_status = eStatusInternal::unloaded;
    header->uid = uid;
    return false;
}

//***************************************
void* Hub::priv_getExistingAssetByUID (const asset::UID &uid)
{
    void *pt = NULL;
    if (knownAssetsList.find(uid, &pt))
        return pt;
    return NULL;
}

//***************************************
bool Hub::getHandle (const char *runtimeName, Handle *out, bool bScheduleLoadNow )
{
    asset::UID uid;
    if (!loader.runtimeNameToUID (runtimeName, &uid))
    {
        gos::logger::err ("asset::Hub::getHandle() => invalid runtineName '%s'\n", runtimeName);
        return false;
    }

    //se l'asset esiste gia'...
    const bool bAlreadyExisting = priv_findOrAddAsset(uid, &out->_pt);
    sHeader *header = static_cast<sHeader*>(out->_pt);

    if (bAlreadyExisting)
        header->numHandleUsingThisAsset++;
    else
    {
        header->numHandleUsingThisAsset = 1;    
    }

    if (bScheduleLoadNow && header->external_status == eStatusPublic::notLoaded)
        priv_scheduleLoad (out->_pt);

    return true;
}

//***************************************
void Hub::priv_scheduleLoad (void *pt)
{
    sHeader *header = static_cast<sHeader*>(pt);
    assert (eStatusPublic::notLoaded == header->external_status);

    //gestisco le dipendenze di questo asset. Se lui dipende da altri, prima devo caricare
    //gli altri
    if (header->uid.getAssetDepth() > 1)
    {
        asset::asset_get_runtime_dependecies_list (*loader.getContext(), header->uid, true, &fastUIDList);
        for (u32 i=0; i<fastUIDList.getNElem(); i++)
        {
            void *ptChild;
            priv_findOrAddAsset(fastUIDList(i), &ptChild);
            sHeader *headerChild = static_cast<sHeader*>(ptChild);

            headerChild->numAssetUsingThissAsset++;

            switch (headerChild->external_status)
            {
            default:
                DBGBREAK;
                header->external_status = eStatusPublic::error;
                return;

            case eStatusPublic::ready:
                break;

            case eStatusPublic::notLoaded:
                headerChild->external_status = eStatusPublic::loading;
                thread::pushMsg (msgq_1W, THREADMSG_1_LOAD, (u64)ptChild);
                break;

            case eStatusPublic::loading:
                //il child e' gia' in fase di caricamento.
                //Non c'e' pericolo ad aggiungere il caricamente di me stesso visto che il thread processa i load in
                //maniera sequenziale. Tempo che arriva a processare il mio caricamento, mio figlio e' gia' online
                break;

            case eStatusPublic::unloading:
                //non so bene come gestiure la cosa, ci pensero' + avanti
                DBGBREAK;
                break;
            
            case eStatusPublic::error:
                //mio figlio e' in errore, devo andare in errore anche io
                header->external_status = eStatusPublic::error;
                return;
            }

        }   
    }

    //schedulo il caricamento di <pt>
    header->external_status = eStatusPublic::loading;
    thread::pushMsg (msgq_1W, THREADMSG_1_LOAD, (u64)pt);
}

//***************************************
void Hub::priv_unload (void *pt)
{
    sHeader *header = static_cast<sHeader*>(pt);
 
    if (eStatusPublic::ready != header->external_status && eStatusPublic::loading != header->external_status)
        return;

    //intanto inizio a unloadere me stesso
    header->external_status = eStatusPublic::unloading;
    thread::pushMsg (msgq_1W, THREADMSG_1_UNLOAD, (u64)pt);

    //se dipendo da qualcuno, vedo se e' il caso di unloadare anche lui
    if (header->uid.getAssetDepth() < 2)
        return;

    asset::asset_get_runtime_dependecies_list (*loader.getContext(), header->uid, true, &fastUIDList);
    for (u32 i=0; i<fastUIDList.getNElem(); i++)
    {
        void *ptToChild = priv_getExistingAssetByUID (fastUIDList(i));
        assert (NULL != ptToChild);

        sHeader *headerChild = static_cast<sHeader*>(ptToChild);
        if (headerChild->numAssetUsingThissAsset > 0)
        {
            headerChild->numAssetUsingThissAsset--;
            if (0 == headerChild->numAssetUsingThissAsset)
                priv_unload(ptToChild);
        }
    }

}