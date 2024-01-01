#include "TTest.h"
#include "gosUtils.h"
#include "gosThreadMsgQ.h"

namespace test_thread
{

struct sUserParam1
{
    i16 paramI16;
};


//*******************************
i16 test1_mainFn (void *userparam)
{
    sUserParam1 *param = reinterpret_cast<sUserParam1*>(userparam);
    param->paramI16++;
    gos::sleep_msec (100);
    return 0;
}

int test1()
{
    gos::Thread hThread1;
    sUserParam1 userParam;
    userParam.paramI16 = 12345;

    //Creo un th, gli passo una struttura e, all'interno del th, modifico il valore di userParam.paramI16
    eThreadError err = gos::thread::create (&hThread1, test_thread::test1_mainFn, &userParam);
    TEST_ASSERT(err == eThreadError::none);
    gos::thread::waitEnd (hThread1);
    TEST_ASSERT(userParam.paramI16 == 12346);

    return 0;
}


//*******************************
struct sTest2_T1
{
    HThreadMsgR hMsgQRead;
    HThreadMsgW hMsgQWrite;
};

i16 test2_mainFn1 (void *userparam)
{
    sTest2_T1 *param = reinterpret_cast<sTest2_T1*>(userparam);
    HThreadMsgR hMsgQRead = param->hMsgQRead;

    //questo thread rimane in attesa di ricevere msg
    gos::Event hEventMsgArrived;
    TEST_ASSERT(true == gos::thread::getMsgQEvent (hMsgQRead, &hEventMsgArrived));

    TEST_ASSERT(true == gos::thread::eventWait (hEventMsgArrived, 3000));

    gos::thread::sMsg msg;
    TEST_ASSERT(true == gos::thread::popMsg (hMsgQRead, &msg));
    TEST_ASSERT(2133 == msg.what);
    TEST_ASSERT(23738 == msg.paramU32);
    TEST_ASSERT(3 == msg.bufferSize);
    TEST_ASSERT(memcmp (msg.buffer, "die", 3) == 0);
    gos::thread::deleteMsg (msg);
    printf ("  test2_mainFn1 => received 'die' mesagge\n");
    return 0;
}

i16 test2_mainFn2 (void *userparam)
{
    sTest2_T1 *param = reinterpret_cast<sTest2_T1*>(userparam);
    HThreadMsgW hMsgQWrite = param->hMsgQWrite;

    //questo thread dorme un attimo, e poi manda un msg di morte all'altro thread
    gos::sleep_msec (400);

    printf ("  test2_mainFn2 => sending 'die' mesagge\n");
    const char msg[3] = {'d', 'i', 'e'};
    gos::thread::pushMsg (hMsgQWrite, 2133, 23738, msg, 3);

    return 0;
}

int test2()
{
    HThreadMsgR hMsgQRead;
    HThreadMsgW hMsgQWrite;
    gos::thread::createMsgQ (&hMsgQRead, &hMsgQWrite);


    //Creo 2 thread e, ad entrambi, passo gli handle della msgQ
    sTest2_T1 userParam;
    userParam.hMsgQRead = hMsgQRead;
    userParam.hMsgQWrite = hMsgQWrite;


    gos::Thread hThread1;
    eThreadError err = gos::thread::create (&hThread1, test_thread::test2_mainFn1, &userParam);
    TEST_ASSERT(err == eThreadError::none);

    gos::Thread hThread2;
    err = gos::thread::create (&hThread2, test_thread::test2_mainFn2, &userParam);
    TEST_ASSERT(err == eThreadError::none);

    gos::thread::waitEnd (hThread1);
    gos::thread::waitEnd (hThread2);;

    gos::thread::deleteMsgQ (hMsgQRead, hMsgQWrite);

    return 0;
}
} //namespace test_thread


//********************************+
void testThread (Tester &tester)
{
    tester.run("thread::test1", test_thread::test1);
    tester.run("thread::test2", test_thread::test2);
}