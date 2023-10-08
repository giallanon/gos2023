#include "TTest.h"
#include "gosHandle.h"

namespace test_gos
{
    //********************************+
    int test_printSystemInfo()
    {
        gos::logger::log ("app folder = %s\n", gos::getAppPathNoSlash());
        gos::logger::log ("app writable folder = %s\n", gos::getPhysicalPathToWritableFolder());
        gos::logger::log ("memory page size = %dB\n", gos::systeminfo::getPageSizeInByte());
        gos::logger::log ("num CPU core = %d\n", gos::systeminfo::getNumOfCPUCore());

        gos::DateTime dt;
        char s[256];
        dt.setNow();
        dt.formatAs_YYYYMMDDHHMMSS (s, sizeof(s));
        gos::logger::log ("current date and time: %s\n", s);

        return 0;
    }

    //********************************************
    int testHandle()
    {
        const u8 NBITS[4] = { 6, 2, 16, 8 };
        const u32 MAX_VALUE_CHUNK[4] = {
            (u32)((0x0001 << NBITS[0])),
            (u32)((0x0001 << NBITS[1])),
            (u32)((0x0001 << NBITS[2])),
            (u32)((0x0001 << NBITS[3])) };

        const u32 DEF_VALUE[4] = { MAX_VALUE_CHUNK[0] - 1, MAX_VALUE_CHUNK[1] - 1 , MAX_VALUE_CHUNK[2] - 1 , MAX_VALUE_CHUNK[3] - 1 };

        gos::HandleT<6,2,16,8> h1;

        //incremento di 1 tutti i singoli canali, uno alla volta e verifico che gli altri canali non ne siano affetti	
        for (u8 n = 0; n < 4; n++)
        {
            h1.setChunkValue(DEF_VALUE[0]); 
            h1.setUserValue(DEF_VALUE[1]); 
            h1.setIndexValue(DEF_VALUE[2]); 
            h1.setCounterValue(DEF_VALUE[3]);
            
            h1.debug_setValueByIndex(n, 0);
            for (u32 i = 0; i < MAX_VALUE_CHUNK[n]; i++)
            {
                TEST_FAIL_IF(h1.debug_getValueByIndex(n) != i);
                h1.debug_incValueByIndex(n);
                
                for (u8 n2 = 0; n2 < 4; n2++)
                {
                    const u32 bitsValue = h1.debug_getValueByIndex(n2);
                    if (n == n2)
                    {
                        TEST_FAIL_IF(bitsValue != ((i + 1) % MAX_VALUE_CHUNK[n]));
                    }
                    else
                    {
                        TEST_FAIL_IF(bitsValue != DEF_VALUE[n2]);
                    }
                }
            }
        }
        return 0;
    }

    //********************************************
    int testHandleArray (gos::Allocator *allocator)
    {
        struct sMyData
        {
            u8	a;
            u16	b;
            u32	c;
            u8 chunk;
        };

        const u8 CHUNKbit	= 2;
        const u8 USERbit	= 11;
        const u8 INDEXbit	= 16;
        const u8 COUNTERbit = 3;
        typedef gos::HandleT< CHUNKbit, USERbit, INDEXbit, COUNTERbit> MyHandle;

        gos::HandleList<MyHandle, sMyData> hl;
        const u32 NMAXHANDLE = MyHandle::getNumMaxHandleAA();
        const u32 NHANDLE_PER_CHUNK = MyHandle::getNumMaxHandlePerChunk();

        sMyData	*myDataList = (sMyData*)GOSALIGNEDALLOC(allocator, NMAXHANDLE * sizeof(sMyData), alignof(sMyData));
        MyHandle *handleList = (MyHandle*)GOSALLOC(allocator, sizeof(MyHandle) * NMAXHANDLE);

        hl.setup(allocator);
            
        //alloco tutti gli handle di chunk 0
        for (u32 i = 0; i < NHANDLE_PER_CHUNK; i++)
        {
            MyHandle handle;
            sMyData *s = hl.reserve(&handle);
            TEST_ASSERT (handle.getChunkValue() == 0);
            TEST_FAIL_IF(!s || handle.isInvalid());
            TEST_FAIL_IF(handle.getCounterValue() != 1);
            TEST_FAIL_IF(handle.getIndexValue() != i);

            s->a = (u8)(i & 0xFF);
            s->b = (u16)(i & 0xFFFF);
            s->c = i;
            s->chunk = 0;

            handleList[i] = handle;
            memcpy(&myDataList[i], s, sizeof(sMyData));
        }

        //verifico chunk 0
        for (u32 i = 0; i < NHANDLE_PER_CHUNK; i++)
        {
            sMyData *s = NULL;
            TEST_FAIL_IF(!hl.fromHandleToPointer(handleList[i], &s));
            TEST_FAIL_IF(memcmp(s, &myDataList[i], sizeof(sMyData)) != 0);
        }

        //alloco qualche handle del chunk1
        for (u32 i = 0; i < NHANDLE_PER_CHUNK; i++)
        {
            MyHandle handle;
            sMyData *s = hl.reserve(&handle);
            TEST_ASSERT (handle.getChunkValue() == 1);
            TEST_FAIL_IF (!s || handle.isInvalid());
            TEST_FAIL_IF (handle.getCounterValue() != 1);
            TEST_FAIL_IF (handle.getIndexValue() != i);

            s->a = (u8)(i & 0xFF);
            s->b = (u16)(i & 0xFFFF);
            s->c = i;
            s->chunk = 1;

            handleList[NHANDLE_PER_CHUNK + i] = handle;
            memcpy(&myDataList[NHANDLE_PER_CHUNK + i], s, sizeof(sMyData));
        }


        //free di tutti gli handle di chunk 0
        for (u32 i = 0; i < NHANDLE_PER_CHUNK; i++)
        {
            MyHandle h = handleList[NHANDLE_PER_CHUNK-i-1];
            hl.release(h);
        }

        //verifico che risultino tutti invalidi
        for (u32 i = 0; i < NHANDLE_PER_CHUNK; i++)
        {
            MyHandle h = handleList[i];
            TEST_ASSERT (h.isValid());

            sMyData* s = NULL;
            TEST_ASSERT (false == hl.fromHandleToPointer(h, &s));
        }


        //allocazioni a caso
        {
            MyHandle h;
            sMyData *s = hl.reserve(&h);
            TEST_FAIL_IF (NULL == s || h.isInvalid());
            TEST_ASSERT (h.getIndexValue() == 0);
            TEST_ASSERT (h.getCounterValue() == 2);
            TEST_ASSERT (h.getChunkValue() == 0);

            MyHandle h2;
            s = hl.reserve(&h2);
            TEST_FAIL_IF(NULL == s || h2.isInvalid());
            TEST_ASSERT (h2.getIndexValue() == 1);
            TEST_ASSERT (h2.getCounterValue() == 2);
            TEST_ASSERT (h.getChunkValue() == 0);

            hl.release(h);
            hl.release(h2);

            s = hl.reserve(&h);
            TEST_FAIL_IF (NULL == s || h.isInvalid());
            TEST_ASSERT (h.getIndexValue() == 1);
            TEST_ASSERT (h.getCounterValue() == 3);
            TEST_ASSERT (h.getChunkValue() == 0);

            s = hl.reserve(&h2);
            TEST_FAIL_IF (NULL == s || h2.isInvalid());
            TEST_ASSERT (h2.getIndexValue() == 0);
            TEST_ASSERT (h2.getCounterValue() == 3);
            TEST_ASSERT (h.getChunkValue() == 0);

            hl.release(h);
            hl.release(h2);
        }

        //verifico chunk 1
        for (u32 i = 0; i < NHANDLE_PER_CHUNK; i++)
        {
            sMyData *s = NULL;
            TEST_FAIL_IF (!hl.fromHandleToPointer(handleList[NHANDLE_PER_CHUNK+i], &s));
            TEST_FAIL_IF (memcmp(s, &myDataList[NHANDLE_PER_CHUNK+i], sizeof(sMyData)) != 0);
        }

        //free di tutti gli handle di chunk 1
        for (u32 i = 0; i < NHANDLE_PER_CHUNK; i++)
        {
            MyHandle h = handleList[NHANDLE_PER_CHUNK+i];
            hl.release(h);
        }

        //verifico che risultino tutti invalidi
        for (u32 i = 0; i < NHANDLE_PER_CHUNK*2; i++)
        {
            MyHandle h = handleList[i];
            TEST_ASSERT (h.isValid());

            sMyData* s = NULL;
            TEST_ASSERT (false == hl.fromHandleToPointer(h, &s));
        }

        GOSFREE(allocator, handleList);
        GOSFREE(allocator, myDataList);
        hl.unsetup();

        return 0;
    }

    //********************************+
    int testFS()
    {
        gos::Allocator *allocator = gos::getSysHeapAllocator();
        u8  s1[2048];
        u8  s2[2048];

        //se la cartella di test esiste, la elimino
        gos::string::utf8::spf (s1, sizeof(s1), "%s/testFS", gos::getPhysicalPathToWritableFolder());
        gos::logger::log ("destination folder is: %s\n", s1);
        if (gos::fs::folderExists(s1))
        {
            gos::fs::folderDeleteAllFileRecursively(s1, eFolderDeleteMode::deleteAlsoTheSubfolderAndTheMainFolder);
            TEST_FAIL_IF (gos::fs::folderExists(s1));
            TEST_FAIL_IF (gos::fs::folderExists(s2));
        }
        gos::fs::folderCreate (s1);

        //creo 4 file in una cartella con sottocartella
        gos::string::utf8::spf (s1, sizeof(s1), "%s/testFS/dir1", gos::getPhysicalPathToWritableFolder());
        const char testoDelFile[] = {"Ciao, sono un file di esempio.\nSeconda riga"};

        for (u8 i=0; i<4; i++)
        {
            gos::File f;
            gos::string::utf8::spf (s2, sizeof(s2), "%s/file_di_esempio%02d.txt", s1, i);
            TEST_ASSERT (gos::fs::fileOpenForW (&f, s2, true));
            gos::fs::fpf (f, testoDelFile);
            gos::fs::fileClose(f);

            //mi assicuro che esista
            TEST_ASSERT(gos::fs::fileExists(s2));

            //lo carico in memoria e faccio memcmp
            u32 n;
            u8 *buffer = gos::fs::fileLoadInMemory(allocator, s2, &n);
            TEST_ASSERT(buffer);
            TEST_ASSERT(n == strlen(testoDelFile));
            TEST_ASSERT(memcmp(buffer, testoDelFile, n) == 0);
            GOSFREE(allocator, buffer);
        }

        //scanno la directory alla ricerca dei 4 file
        gos::FileFind ff;
        u32 nFound = 0;
        TEST_ASSERT(gos::fs::findFirst (&ff, s1, "*.txt"));
        do
        {
            if (gos::fs::findIsDirectory(ff))
                continue;

            char utc[128];
            char localTime[128];
            gos::DateTime dt;
            gos::string::utf8::spf (s2, sizeof(s2), "%s/%s", s1, gos::fs::findGetFileName(ff));
            gos::fs::fileGetLastTimeModified_UTC (s2, &dt);        
            dt.formatAs_YYYYMMDDHHMMSS(utc, sizeof(utc));
            gos::fs::fileGetLastTimeModified_LocalTime (s2, &dt);        
            dt.formatAs_YYYYMMDDHHMMSS(localTime, sizeof(localTime));

            gos::logger::log ("file found: %s, last time modified: UTC: %s, localTime: %s \n", gos::fs::findGetFileName(ff), utc, localTime);
            nFound++;
        } while (gos::fs::findNext(ff));
        gos::fs::findClose(ff);
        TEST_FAIL_IF(nFound!=4);


    return 0;
    }
} //namespace test_gos

//********************************+
void testGos()
{
    TEST("gos::system info", test_gos::test_printSystemInfo);
    TEST("gos::handle", test_gos::testHandle);
    TEST("gos::handle array", test_gos::testHandleArray, gos::getSysHeapAllocator());
    TEST("gos::testFS", test_gos::testFS);
}