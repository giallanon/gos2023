#ifndef _Server_h_
#define _Server_h_
#include "gos.h"
#include "gosServerTCP.h"


/*****************************************
 * Server
 * 
*/
class Server
{
public:
        Server();
        ~Server();

    void    run ();
    void    quit()      { bQuit = true; }

private:
    static const u16 TCP_PORT = 3765;

private:
    bool                bQuit;
    gos::ServerTCP      *server;
    gos::ProtocolBuffer bufferR;
    

}; //class Server

#endif //_Server_h_


