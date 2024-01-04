#include "gos.h"
#include "gosUtils.h"
#include "TTest.h"

void testGos (Tester &tester);
void testMath(Tester &tester);
void testThread (Tester &tester);

//********************************
#include "protocol/gosProtocolChSocketTCP.h"
#include "protocol/gosProtocolConsole.h"
void testTCP()
{
    //tento la connessione
    gos::Socket sok;
    {
        gos::socket::init (&sok);
        eSocketError err =  gos::socket::openAsTCPClient (&sok, "127.0.0.1", 3765);
        if (eSocketError::none != err)
        {
            gos::logger::err ("%s\n", gos::utils::enumToString (err));
            return;
        }
    }

    //bindo la socket al channel
    gos::ProtocolChSocketTCP channel(gos::getSysHeapAllocator(), 1024, 8192);
    channel.bindSocket(sok);

    //handshake
    gos::logger::log ("handshake\n");
    gos::ProtocolConsole protocol (gos::getSysHeapAllocator(), 1024);
    if (!protocol.handshake_clientSend (&channel, gos::logger::getSystemLogger()))
    {
            gos::logger::err ("unable to handshake\n");
            return;
    }
    gos::logger::log (eTextColor::green, "Connected!\n");

    //siamo dentro!
    protocol.write (&channel, (const u8*)"ciao", 4, 2000);
    protocol.close (&channel);
}

//********************************
void runAllTest()
{
    Tester tester;
    {
        testGos (tester);
        testMath(tester);
        testThread (tester);
    }
    tester.printReport();
}


//********************************+
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForNonGame();
    //init.writableFolder_setMode (gos::sGOSInit::eWritableFolder::inUserFolder);
    if (!gos::init (init, "testKernel"))
        return 1;

    //testTCP();
    runAllTest();


    gos::deinit();

#ifdef GOS_PLATFORM__WINDOWS
    printf ("\nPress any key to terminate\n");
    _getch();
#endif

    return 0;
}