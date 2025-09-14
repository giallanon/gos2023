#include "TTest.h"
#include "gosUtils.h"
#include "gosThreadMsgQ.h"
#include "gosWaitableGrp.h"

namespace test_thread
{


//*******************************
namespace test1
{
    struct sUserParam1
    {
        i16 paramI16;
    };

    i16 mainFn (void *userparam)
    {
        sUserParam1 *param = reinterpret_cast<sUserParam1*>(userparam);
        param->paramI16++;
        gos::sleep_msec (100);
        return 0;
    }

    int run()
    {
        GOSThreadHandle hThread1;
        sUserParam1 userParam;
        userParam.paramI16 = 12345;

        //Creo un th, gli passo una struttura e, all'interno del th, modifico il valore di userParam.paramI16
        eThreadError err = gos::thread::create (&hThread1, test_thread::test1::mainFn, &userParam);
        TEST_ASSERT(err == eThreadError::none);
        gos::thread::waitEnd (hThread1);
        TEST_ASSERT(userParam.paramI16 == 12346);

        return 0;
    }
}

//*******************************
namespace test2
{
    struct sUserParam
    {
        HThreadMsgR hMsgQRead;
        HThreadMsgW hMsgQWrite;
    };

    i16 mainFn1 (void *userparam)
    {
        sUserParam *param = reinterpret_cast<sUserParam*>(userparam);
        HThreadMsgR hMsgQRead = param->hMsgQRead;

        //questo thread rimane in attesa di ricevere msg
        /*gos::Event hEventMsgArrived;
        TEST_ASSERT(true == gos::thread::getMsgQEvent (hMsgQRead, &hEventMsgArrived));
        TEST_ASSERT(true == gos::thread::eventWait (hEventMsgArrived, 3000));
        */
        TEST_ASSERT(true == gos::thread::waitForAnEvent(hMsgQRead, 3000));

        gos::thread::sMsg msg;
        TEST_ASSERT(true == gos::thread::popMsg (hMsgQRead, &msg));
        TEST_ASSERT(2133 == msg.what);
        TEST_ASSERT(23738 == msg.paramU64);
        TEST_ASSERT(3 == msg.bufferSize);
        TEST_ASSERT(memcmp (msg.buffer, "die", 3) == 0);
        gos::thread::deleteMsg (msg);
        printf ("  test2::mainFn1 => received 'die' message\n");
        return 0;
    }

    i16 mainFn2 (void *userparam)
    {
        sUserParam *param = reinterpret_cast<sUserParam*>(userparam);
        HThreadMsgW hMsgQWrite = param->hMsgQWrite;

        //questo thread dorme un attimo, e poi manda un msg di morte all'altro thread
        gos::sleep_msec (400);

        printf ("  test2::mainFn2 => sending 'die' message\n");
        const char msg[3] = { 'd', 'i', 'e' };
        gos::thread::pushMsg (hMsgQWrite, 2133, 23738, msg, 3);
        printf ("  test2::mainFn2 => msg sent\n");

        return 0;
    }

    int run()
    {
        HThreadMsgR hMsgQRead;
        HThreadMsgW hMsgQWrite;
        gos::thread::createMsgQ (&hMsgQRead, &hMsgQWrite);


        //Creo 2 thread e, ad entrambi, passo gli handle della msgQ
        sUserParam userParam;
        userParam.hMsgQRead = hMsgQRead;
        userParam.hMsgQWrite = hMsgQWrite;


        GOSThreadHandle hThread1;
        eThreadError err = gos::thread::create (&hThread1, test_thread::test2::mainFn1, &userParam);
        TEST_ASSERT(err == eThreadError::none);

        GOSThreadHandle hThread2;
        err = gos::thread::create (&hThread2, test_thread::test2::mainFn2, &userParam);
        TEST_ASSERT(err == eThreadError::none);

        gos::thread::waitEnd (hThread1);
        printf ("thread1 finished\n");
        
        gos::thread::waitEnd (hThread2);
        printf ("thread2 finished\n");

        gos::thread::deleteMsgQ (hMsgQRead, hMsgQWrite);

        return 0;
    }
}

//*******************************
namespace test3
{
    struct sUserParam
    {
        HThreadMsgR hMsgQRead;
        HThreadMsgW hMsgQWrite;
    };

    i16 mainFn1 (void *userparam)
    {
        sUserParam *param = reinterpret_cast<sUserParam*>(userparam);
        HThreadMsgR hMsgQRead = param->hMsgQRead;

        //questo thread rimane in attesa di ricevere msg
        gos::WaitableGrp waitable;
        TEST_ASSERT(true == waitable.addMsgQ (hMsgQRead, 675));

        u8 nEvents = waitable.wait(3000);
        TEST_ASSERT(nEvents == 1);
        TEST_ASSERT(eWaitEventOrigin::msgQ == waitable.getEventOrigin(0));
        TEST_ASSERT(675 == waitable.getEventUserParamAsU32(0));

        HThreadMsgR hRead = waitable.getEventSrcAsMsgQ(0);
        TEST_ASSERT(hRead == hMsgQRead);

        gos::thread::sMsg msg;
        TEST_ASSERT(true == gos::thread::popMsg (hRead, &msg));
        TEST_ASSERT(2133 == msg.what);
        TEST_ASSERT(23738 == msg.paramU64);
        TEST_ASSERT(3 == msg.bufferSize);
        TEST_ASSERT(memcmp (msg.buffer, "die", 3) == 0);
        gos::thread::deleteMsg (msg);
        printf ("  test3::mainFn1 => received 'die' message\n");
        return 0;
    }

    i16 mainFn2 (void *userparam)
    {
        sUserParam *param = reinterpret_cast<sUserParam*>(userparam);
        HThreadMsgW hMsgQWrite = param->hMsgQWrite;

        //questo thread dorme un attimo, e poi manda un msg di morte all'altro thread
        gos::sleep_msec (400);

        printf ("  test3::mainFn2 => sending 'die' message\n");
        const char msg[3] = { 'd', 'i', 'e' };
        gos::thread::pushMsg (hMsgQWrite, 2133, 23738, msg, 3);

        return 0;
    }

    int run()
    {
        HThreadMsgR hMsgQRead;
        HThreadMsgW hMsgQWrite;
        gos::thread::createMsgQ (&hMsgQRead, &hMsgQWrite);


        //Creo 2 thread e, ad entrambi, passo gli handle della msgQ
        sUserParam userParam;
        userParam.hMsgQRead = hMsgQRead;
        userParam.hMsgQWrite = hMsgQWrite;


        GOSThreadHandle hThread1;
        eThreadError err = gos::thread::create (&hThread1, test_thread::test3::mainFn1, &userParam);
        TEST_ASSERT(err == eThreadError::none);

        GOSThreadHandle hThread2;
        err = gos::thread::create (&hThread2, test_thread::test3::mainFn2, &userParam);
        TEST_ASSERT(err == eThreadError::none);

        gos::thread::waitEnd (hThread1);
        gos::thread::waitEnd (hThread2);;

        gos::thread::deleteMsgQ (hMsgQRead, hMsgQWrite);

        return 0;
    }
}

//*******************************
namespace test4_globalErr
{
    struct sUserParam1
    {
        i16 paramI16;
    };

    i16 mainFn (void *userparam)
    {
        sUserParam1 *param = reinterpret_cast<sUserParam1*>(userparam);
        
        TEST_ASSERT(gos::err::anyError() == 0);
        printf ("th%d: error set\n", param->paramI16);
        gos::err::add ("errore dal main thread %d\n", param->paramI16);
        
        TEST_ASSERT(gos::err::anyError() == 1);
        
        gos::sleep_msec (100);
        return 0;
    }

    int run()
    {
        //Creo un th1
        GOSThreadHandle hThread1;
        sUserParam1 userParam1;
        userParam1.paramI16 = 1;
        eThreadError err = gos::thread::create (&hThread1, test_thread::test4_globalErr::mainFn, &userParam1);
        TEST_ASSERT(err == eThreadError::none);

        gos::sleep_msec (100);

        //Creo un th2
        GOSThreadHandle hThread2;
        sUserParam1 userParam2;
        userParam2.paramI16 = 2;
        err = gos::thread::create (&hThread2, test_thread::test4_globalErr::mainFn, &userParam2);
        TEST_ASSERT(err == eThreadError::none);


        TEST_ASSERT(gos::err::anyError() == 0);
        gos::sleep_msec (200);


        //a questo punto sia th1 che th2 dovrebbero aver settato il loro errore, ma a me non deve risultare nessun errore
        TEST_ASSERT(gos::err::anyError() == 0);

        gos::err::add ("errore dal main thread");
        TEST_ASSERT(gos::err::anyError() == 1);

        gos::thread::waitEnd (hThread1);
        TEST_ASSERT(gos::err::anyError() == 1);


        gos::thread::waitEnd (hThread2);
        TEST_ASSERT(gos::err::anyError() == 1);

        gos::err::clear();
        TEST_ASSERT(gos::err::anyError() == 0);



        return 0;
    }
}

} //namespace test_thread


//********************************+
void testThread (Tester &tester)
{
    tester.run("thread::test1", test_thread::test1::run);
    tester.run("thread::test2", test_thread::test2::run);
    tester.run("thread::test3", test_thread::test3::run);
    tester.run("thread::test4_globalErr", test_thread::test4_globalErr::run);
}