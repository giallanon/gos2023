#include "gosAsset2Hub.h"
#include "gosAsset2.h"


using namespace gos;
using namespace gos::asset2;


//***************************************
void	Hub::ThreadFN_changeStatus (const HThreadMsgW &msgqW, sHeader *header, eStatusPublic newStatus)
{
    thread::pushMsg (msgqW, THREADMSG_2_CHANGE_STATUS, (u64)header, NULL, static_cast<u32>(newStatus));
}

//***************************************
i16	Hub::ThreadFN_main (void *paramsIN)
{
    HThreadMsgR         msgqR;
    HThreadMsgW         msgqW;
    asset2::Loader		*loader = NULL;
    gos::Logger			*logger;

    //copia dei params
    {
        const sThreadParams *params = reinterpret_cast<const sThreadParams*>(paramsIN);
        msgqR = params->msgqR;
        msgqW = params->msgqW;
        loader = params->loader;
        logger = params->logger;

        //segnalo che sono partito
        thread::eventFire (params->hEvent_started);
    }


    bool bQuit = false;
    while (bQuit == false)
    {
        if (!thread::waitForAnEvent (msgqR, u32MAX))
            break;

        static constexpr u8 NUM_MAX_MESSAGES = 16;
        thread::sMsg msgList[NUM_MAX_MESSAGES];
        u32 nMsg;
        while (0 != (nMsg = thread::popMultipleMsg(msgqR, msgList, NUM_MAX_MESSAGES)))
        {
            for (u32 i=0; i<nMsg; i++)
            {
                void *pt = (void*)msgList[i].paramU64;
                sHeader *header = static_cast<sHeader*>(pt);

                switch (msgList[i].what)
                {
                default:
                    DBGBREAK;
                    break;

                case THREADMSG_1_DIE:
                    bQuit = true;
                    break;

                case THREADMSG_1_LOAD:
                    if (header->internal_status == eStatusInternal::unloaded)
                    {
                        void *ptToAssetData = (void*) (static_cast<u8*>(pt) + sizeof(sHeader));
                        if (loader->load (header->uid, ptToAssetData))
                        {
                            header->internal_status = eStatusInternal::loaded;
                            ThreadFN_changeStatus (msgqW, header, eStatusPublic::ready);
                        }
                        else
                        {
                            header->internal_status = eStatusInternal::error;
                            ThreadFN_changeStatus (msgqW, header, eStatusPublic::error);
                            logger->err ("asset2::Hub::ThreadFN_main() => LOAD => error loading asset %016" PRIX64 "\n");
                        }
                    }
                    break;

                case THREADMSG_1_UNLOAD:
                    if (header->internal_status == eStatusInternal::loaded)
                    {
                        void *ptToAssetData = (void*) (static_cast<u8*>(pt) + sizeof(sHeader));
                        
                        header->internal_status = eStatusInternal::unloaded;
                        loader->unload (header->uid, ptToAssetData);
                        ThreadFN_changeStatus (msgqW, header, eStatusPublic::notLoaded);
                    }
                    break;
                }

                thread::deleteMsg (msgList[i]);
            }

            if (bQuit)
                break;
        }
    }

    //TODO:: eliminare tutti i messaggi pendendi nel caso in cui ci siano ancora
    //risorse da unloadare

    return 0;
}

