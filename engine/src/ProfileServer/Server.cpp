#include "Server.h"

//*************************************
Server::Server()
{
    server = NULL;
    bQuit = false;
    bufferR.setup (gos::getSysHeapAllocator(), 10 * 1024);
}

//*************************************
Server::~Server()
{
    bufferR.unsetup();
}

//*************************************
void Server::run ()
{
    server = GOSNEW(gos::getSysHeapAllocator(), gos::ServerTCP) (1, gos::getSysHeapAllocator());
    server->useLogger (gos::logger::getSystemLogger());
    if (eSocketError::none != server->start (TCP_PORT))
    {
        gos::logger::err ("Can't open server on port %d\n", TCP_PORT);       
        GOSDELETE(gos::getSysHeapAllocator(), server);
        server = NULL;
        return;
    }

    //main loop
    bQuit = false;
    while (bQuit == false)
    {
        const u8 nEvents = server->wait (u32MAX);

        //gos::logger::log ("found %d events\n", nEvents);
        for (u8 i=0; i<nEvents; i++)
        {
            switch (server->getEventType(i))
            {
            default:
                gos::logger::log ("unhandled event of type %d\n", (u8)server->getEventType(i));
                break;

            case gos::ServerTCP::eEventType::new_client_connected:
                {
                    gos::logger::log ("new_client_connected\n");
                    //HSokServerClient h = server->getEventSrcAsClientHandle(i);
                    
                }
                break;

            case gos::ServerTCP::eEventType::client_has_data_avail:
                {
                    gos::logger::log ("client_has_data_avail\n");
                    HSokServerClient h = server->getEventSrcAsClientHandle(i);
                    server->client_read (h, bufferR);
                    
                }
                break;
            }
        }
    }

    //fine
    server->close();
    GOSDELETE(gos::getSysHeapAllocator(), server);
    server = NULL;
}
