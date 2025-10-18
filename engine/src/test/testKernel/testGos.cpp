#include "TTest.h"
#include "gosHandle.h"
#include "string/gosStringList.h"
#include "gosUtils.h"
#include "gosBit.h"
#include "gosImage.h"

namespace test_gos
{

namespace test_assertion_helpers
{
    int run()
    {
        static_assert (GOS_IS_POWER_OF_TWO<u32>(1));
        static_assert (GOS_IS_POWER_OF_TWO<u32>(2));
        static_assert (false == GOS_IS_POWER_OF_TWO<u32>(3));
        static_assert (GOS_IS_POWER_OF_TWO<u32>(4));


        static_assert (GOS_NEXT_POWER_OF_TWO<u32>(120) == 128);
        static_assert (GOS_NEXT_POWER_OF_TWO<u32>(3) == 4);
        static_assert (GOS_NEXT_POWER_OF_TWO<u32>(1000) == 1024);


        static_assert (GOS_FAST_MOD<u32>(1000, 256) == 232);


        return 0;
    }    
};


//********************************************
namespace test1
{
    int test_printSystemInfo()
    {
        gos::logger::log ("app folder = %s\n", gos::getAppPathNoSlash());
        gos::logger::log ("app writable folder = %s\n", gos::getPhysicalPathToWritableFolder());
        gos::logger::log ("memory page size = %dB\n", gos::systeminfo::getPageSizeInByte());
        gos::logger::log ("num CPU core = %d\n", gos::systeminfo::getNumOfCPUCore());

        gos::DateTime dt;
        char s[256];
        dt.setNow_local();
        dt.formatAs_YYYYMMDDHHMMSS (s, sizeof(s));
        gos::logger::log ("current date and time: %s\n", s);

        return 0;
    }
} //namespace test1

//********************************************
namespace test2
{
    template<u32 A, u32 B, u32 C>
    int testHandle()
    {
        constexpr u32 D = 32 - (A+B+C);
        const u32 NUM_MAX[4] = {
            (u32)((0x0001 << A)),
            (u32)((0x0001 << B)),
            (u32)((0x0001 << C)),
            (u32)((0x0001 << D)) 
        };

        const u32 MAX_VALUE[4] = {
            (u32)((0x0001 << A) - 1),
            (u32)((0x0001 << B) - 1),
            (u32)((0x0001 << C) - 1),
            (u32)((0x0001 << D) - 1) 
        };


        gos::HandleT<A, B, C> handle1;

        //incremento di 1 tutti i singoli canali, uno alla volta e verifico che gli altri canali non ne siano affetti	
        for (u8 channelNum = 0; channelNum < 4; channelNum++)
        {
            //metto tutti i canali al loro valore massimo
            //e poi muovo 1 canale alla volta a partire dal valore 0 fino al suo massimo
            //Lo scopo e' vedere che quel canale si incrementa correttamente e che gli altri canali mantengono
            //il loro valore originale
            handle1.set_indexValue (MAX_VALUE[0]); 
            handle1.set_chunkValue (MAX_VALUE[1]); 
            handle1.set_counterValue(MAX_VALUE[2]);
            handle1.set_extraValue (MAX_VALUE[3]); 
            
            u32 testValue = 0;
            handle1.debug_setValueByIndex (channelNum, testValue);
            for (u32 i = 0; i < NUM_MAX[channelNum]; i++)
            {
                TEST_FAIL_IF(handle1.debug_getValueByIndex(channelNum) != testValue);
                handle1.debug_incValueByIndex(channelNum);
                testValue++;
                
                for (u8 n2 = 0; n2 < 4; n2++)
                {
                    if (n2 != channelNum)
                    {
                        const u32 channelValue = handle1.debug_getValueByIndex(n2);
                        TEST_FAIL_IF(channelValue != MAX_VALUE[n2]);
                    }
                }
            }

            //Come sopra, ma stavolta metto tutti i canali a 0
            handle1.set_indexValue (0); 
            handle1.set_chunkValue (0); 
            handle1.set_counterValue(0);
            handle1.set_extraValue (0); 
            
            testValue = 0;
            handle1.debug_setValueByIndex (channelNum, testValue);
            for (u32 i = 0; i < NUM_MAX[channelNum]; i++)
            {
                TEST_FAIL_IF(handle1.debug_getValueByIndex(channelNum) != testValue);
                handle1.debug_incValueByIndex(channelNum);
                testValue++;
                
                for (u8 n2 = 0; n2 < 4; n2++)
                {
                    if (n2 != channelNum)
                    {
                        const u32 channelValue = handle1.debug_getValueByIndex(n2);
                        TEST_FAIL_IF(channelValue != 0);
                    }
                }
            }

        }

        return 0;
    }

    int run ()
    {
        int err;
        
        err = testHandle<16,8,8>(); TEST_ASSERT(err==0);
        err = testHandle<16,7,8>(); TEST_ASSERT(err==0);
        err = testHandle<16,5,8>(); TEST_ASSERT(err==0);
        err = testHandle<16,4,8>(); TEST_ASSERT(err==0);
        err = testHandle<16,3,8>(); TEST_ASSERT(err==0);
        err = testHandle<16,2,8>(); TEST_ASSERT(err==0);
        err = testHandle<16,1,8>(); TEST_ASSERT(err==0);

        err = testHandle<10,6,16>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,15>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,14>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,13>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,12>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,11>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,10>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,9>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,8>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,7>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,6>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,5>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,4>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,3>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,2>(); TEST_ASSERT(err==0);
        err = testHandle<10,6,1>(); TEST_ASSERT(err==0);

        return 0;
    }
}

//********************************************
namespace test3
{
    int testHandleArray ()
    {
        gos::Allocator *allocator = gos::getSysHeapAllocator();
        struct sMyData
        {
            u8	a;
            u16	b;
            u32	c;
            u8 chunk;
        };

        const u8 INDEXbit	= 16;
        const u8 CHUNKbit	= 2;
        const u8 COUNTERbit = 3;
        GOS_DECL_HANDLE(INDEXbit, CHUNKbit, COUNTERbit, MyHandle);

        gos::HandleList<MyHandle, sMyData> hl;
        const u32 NMAXHANDLE = MyHandle::getNumMaxHandle();
        const u32 NHANDLE_PER_CHUNK = MyHandle::getNumMaxHandlePerChunk();

        sMyData	*myDataList = (sMyData*)GOSALIGNEDALLOC(allocator, NMAXHANDLE * sizeof(sMyData), alignof(sMyData));
        MyHandle *handleList = (MyHandle*)GOSALLOC(allocator, sizeof(MyHandle) * NMAXHANDLE);

        hl.setup(allocator);
            
        //alloco tutti gli handle di chunk 0
        for (u32 i = 0; i < NHANDLE_PER_CHUNK; i++)
        {
            MyHandle handle;
            sMyData *s = hl.reserve(&handle);
            TEST_ASSERT (handle._handle.get_chunkValue() == 0);
            TEST_FAIL_IF(!s || handle.isInvalid());
            TEST_FAIL_IF(handle._handle.get_counterValue() != 1);
            TEST_FAIL_IF(handle._handle.get_indexValue() != i);

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
            TEST_ASSERT (handle._handle.get_chunkValue() == 1);
            TEST_FAIL_IF (!s || handle.isInvalid());
            TEST_FAIL_IF (handle._handle.get_counterValue() != 1);
            TEST_FAIL_IF (handle._handle.get_indexValue() != i);

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
            TEST_ASSERT (h._handle.get_indexValue() == 0);
            TEST_ASSERT (h._handle.get_counterValue() == 2);
            TEST_ASSERT (h._handle.get_chunkValue() == 0);

            MyHandle h2;
            s = hl.reserve(&h2);
            TEST_FAIL_IF(NULL == s || h2.isInvalid());
            TEST_ASSERT (h2._handle.get_indexValue() == 1);
            TEST_ASSERT (h2._handle.get_counterValue() == 2);
            TEST_ASSERT (h._handle.get_chunkValue() == 0);

            hl.release(h);
            hl.release(h2);

            s = hl.reserve(&h);
            TEST_FAIL_IF (NULL == s || h.isInvalid());
            TEST_ASSERT (h._handle.get_indexValue() == 1);
            TEST_ASSERT (h._handle.get_counterValue() == 3);
            TEST_ASSERT (h._handle.get_chunkValue() == 0);

            s = hl.reserve(&h2);
            TEST_FAIL_IF (NULL == s || h2.isInvalid());
            TEST_ASSERT (h2._handle.get_indexValue() == 0);
            TEST_ASSERT (h2._handle.get_counterValue() == 3);
            TEST_ASSERT (h._handle.get_chunkValue() == 0);

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
}

//********************************+
namespace test4
{
    int testFS()
    {
        gos::Allocator *allocator = gos::getSysHeapAllocator();
        char  s1[2048];
        char  s2[2048];

        //se la cartella di test esiste, la elimino
        gos::string::utf8::spf (s1, sizeof(s1), "%s/testFS", gos::getPhysicalPathToWritableFolder());
        gos::logger::log ("destination folder is: %s\n", s1);
        if (gos::fs::folderExists(s1))
        {
            gos::fs::folderDeleteAllFileRecursively(s1, eFolderDeleteMode::deleteAlsoTheSubfolderAndTheMainFolder);
            TEST_FAIL_IF (gos::fs::folderExists(s1));
        }

        //e poi la ricreo...
        gos::fs::folderCreate (s1);


        //creo 4 file in una cartella con sottocartella
        gos::string::utf8::spf (s1, sizeof(s1), "%s/testFS/dir1", gos::getPhysicalPathToWritableFolder());
        const char testoDelFile[] = {"Ciao, sono un file di esempio.\nSeconda riga"};

        for (u8 i=0; i<4; i++)
        {
            gos::string::utf8::spf (s2, sizeof(s2), "%s/file_di_esempio%02d.txt", s1, i);

            //mi assicuro che non esista (perche' ho cancellato la cartella di test all'inizio)
            TEST_ASSERT(false == gos::fs::fileExists(s2));

            //lo creo
            gos::File f;
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


        //verifico che la risoluzione dei path speciali funzioni
        //Il carattere speciale "@" l'ho gia' testato prima, ora invece verifico che path che non iniziano con "/"
        //vengano automaticamente prefissi con il path dell'app
        TEST_ASSERT(gos::fs::findFirst (&ff, gos::getAppPathNoSlash(), "*"));
        do
        {
            if (gos::fs::findIsDirectory(ff))
                continue;


            //ho trovato un file nella cartella dell'app.
            //Verifico di trovarlo anche unsando un path relativo
            gos::string::utf8::spf (s2, sizeof(s2), "%s", gos::fs::findGetFileName(ff));
            TEST_ASSERT (gos::fs::fileExists(s2));

        } while (gos::fs::findNext(ff));
        gos::fs::findClose(ff);        
        

        //verifico il fn degli alias
        TEST_ASSERT (gos::fs::fileExists ("@w/testFS/dir1/file_di_esempio00.txt"));

        TEST_ASSERT (gos::fs::addAlias ("@fs1", "testFS", eAliasPathMode::relativeToWritableFolder));
        TEST_ASSERT (gos::fs::fileExists ("@fs1/dir1/file_di_esempio00.txt"));

        TEST_ASSERT (gos::fs::addAlias ("@fs2", "testFS/dir1", eAliasPathMode::relativeToWritableFolder));
        TEST_ASSERT (gos::fs::fileExists ("@fs2/file_di_esempio00.txt"));

        gos::string::utf8::spf (s1, sizeof(s1), "%s/testFS/dir1", gos::getPhysicalPathToWritableFolder());
        TEST_ASSERT (gos::fs::addAlias ("@fs3", s1, eAliasPathMode::absolutePath));
        TEST_ASSERT (gos::fs::fileExists ("@fs3/file_di_esempio00.txt"));

    return 0;
    }
}

//********************************************
namespace test5
{
    int testStringList()
    {
        gos::Allocator *allocator = gos::getSysHeapAllocator();
        gos::StringList sl;

        sl.setup (allocator, 16);
        const u32 offset1 = sl.add ("Pippo");    
        const u32 offset2 = sl.add ("Pluto");

        //l'inserimento di questa stringa causa una espazione della stringlist
        const u32 offset3 = sl.add ("paperino");

        TEST_ASSERT(sl.getNumString() == 3);

        TEST_ASSERT(strcmp (sl.getStringAtOffset(offset1), "Pippo") == 0);
        TEST_ASSERT(strcmp (sl.getStringAtOffset(offset2), "Pluto") == 0);
        TEST_ASSERT(strcmp (sl.getStringAtOffset(offset3), "paperino") == 0);

        return 0;
    }
}

//********************************************
namespace test6
{
    int testBitUtils()
    {
        {
            static const u8     BUFFER_SIZE_BYTE = 32;
            static const u32    NUM_BIT = BUFFER_SIZE_BYTE * 8;
            u8 dst[BUFFER_SIZE_BYTE];

            gos::bitZERO (dst, sizeof(dst));
            for (u32 i=0; i<BUFFER_SIZE_BYTE; i++)
            {
                TEST_ASSERT(dst[i] == 0);
            }

            for (u32 i=0; i<NUM_BIT; i++)
            {
                //setto 1 bit alla volta
                gos::bitSET (dst, BUFFER_SIZE_BYTE, i);

                //verifico che tutti i primi [i] bit siano a 1
                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(gos::isBitSET(dst, BUFFER_SIZE_BYTE, i2));
                }
                
                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BIT; i2++)
                {
                    TEST_ASSERT(!gos::isBitSET(dst, BUFFER_SIZE_BYTE, i2));
                }            
            }
        }

        {
            static const u8     BUFFER_SIZE_BYTE = 32;
            static const u32    NUM_BIT = BUFFER_SIZE_BYTE * 8;

            gos::Bitfield bf;

            bf.setup (gos::getSysHeapAllocator(), NUM_BIT);
            bf.zero();

            const u8 *dst = reinterpret_cast<const u8*>(bf.getBuffer());
            for (u32 i=0; i<BUFFER_SIZE_BYTE; i++)
            {
                TEST_ASSERT(dst[i] == 0);
            }

            for (u32 i=0; i<NUM_BIT; i++)
            {
                //setto 1 bit alla volta
                bf.set (i);

                //verifico che tutti i primi [i] bit siano a 1
                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(bf.isBitSet(i2));
                    TEST_ASSERT(gos::isBitSET(dst, BUFFER_SIZE_BYTE, i2));
                }
                
                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BIT; i2++)
                {
                    TEST_ASSERT(!bf.isBitSet(i2));
                    TEST_ASSERT(!gos::isBitSET(dst, BUFFER_SIZE_BYTE, i2));
                }            
            }

            bf.zero();
            for (u32 i=0; i<NUM_BIT; i++)
            {
                u32 index;
                TEST_ASSERT(bf.findAndSetFirstFreeBit(&index));
                TEST_ASSERT(index == i);

                if (i == NUM_BIT-1)
                {
                    TEST_ASSERT(false == bf.findFirstFreeBit (0, &index));
                }
                else
                {
                    TEST_ASSERT(bf.findFirstFreeBit (i+1, &index));
                    TEST_ASSERT(index == i+1);

                    TEST_ASSERT (bf.findFirstSetBit(i, &index));
                    TEST_ASSERT(index == i);
                }
            }

            u32 index;
            TEST_ASSERT(false == bf.findAndSetFirstFreeBit(&index));

            bf.unsetup (gos::getSysHeapAllocator());
        }

        //ripeto quanto fatto sopra ma per il caso u32
        {
            static const u8 NUM_BIT = 32;
            
            u32 dst = 0;
            for (u32 i=0; i<NUM_BIT; i++)
            {
                //setto 1 bit alla volta
                gos::bit32SET (&dst, i);

                //verifico che tutti i primi [i] bit siano a 1
                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(gos::isBit32SET(dst, i2));
                }
                
                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BIT; i2++)
                {
                    TEST_ASSERT(!gos::isBit32SET(dst, i2));
                }            
            }
        }
        

        //ripeto quanto fatto sopra ma per il caso u8
        {
            static const u8 NUM_BIT = 8;
            gos::Flag8 flag;
            
            flag.zero();
            TEST_ASSERT(flag.getBitmask() == 0);

            for (u32 i=0; i<NUM_BIT; i++)
            {
                //setto 1 bit alla volta
                flag.set(i);

                //verifico che tutti i primi [i] bit siano a 1
                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(flag.isBitSet(i2));
                }
                
                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BIT; i2++)
                {
                    TEST_ASSERT(!flag.isBitSet(i2));
                }            
            }
        }

        //ripeto quanto fatto sopra ma per il caso u16
        {
            static const u8 NUM_BIT = 16;
            gos::Flag16 flag;
            
            flag.zero();
            TEST_ASSERT(flag.getBitmask() == 0);

            for (u32 i=0; i<NUM_BIT; i++)
            {
                //setto 1 bit alla volta
                flag.set(i);

                //verifico che tutti i primi [i] bit siano a 1
                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(flag.isBitSet(i2));
                }
                
                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BIT; i2++)
                {
                    TEST_ASSERT(!flag.isBitSet(i2));
                }            
            }
        }

        //ripeto quanto fatto sopra ma per il caso u32
        {
            static const u8 NUM_BIT = 32;
            gos::Flag32 flag;
            
            flag.zero();
            TEST_ASSERT(flag.getBitmask() == 0);

            for (u32 i=0; i<NUM_BIT; i++)
            {
                //setto 1 bit alla volta
                flag.set(i);

                //verifico che tutti i primi [i] bit siano a 1
                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(flag.isBitSet(i2));
                }
                
                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BIT; i2++)
                {
                    TEST_ASSERT(!flag.isBitSet(i2));
                }            
            }
        }       

        //ripeto quanto fatto sopra ma per il caso u64
        {
            static const u8 NUM_BIT = 64;
            gos::Flag64 flag;
            
            flag.zero();
            TEST_ASSERT(flag.getBitmask() == 0);

            for (u32 i=0; i<NUM_BIT; i++)
            {
                //setto 1 bit alla volta
                flag.set(i);

                //verifico che tutti i primi [i] bit siano a 1
                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(flag.isBitSet(i2));
                }

                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BIT; i2++)
                {
                    TEST_ASSERT(!flag.isBitSet(i2));
                }            
            }
        }         
        

        //test per il set di fn byteSET/GET
        {
            static const u8 NUM_BYTE = 4;
            
            u8 byteValueList[NUM_BYTE];
            for (u32 i=0; i<NUM_BYTE; i++)
                byteValueList[i] = static_cast<u8>(0x32 + i);
            
            u32 dst;
            dst = 0;

            for (u32 i=0; i<NUM_BYTE; i++)
            {
                gos::byte32SET (&dst, byteValueList[i], i);

                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(gos::byte32GET(dst, i2) == byteValueList[i2]);
                }
                
                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BYTE; i2++)
                {
                    TEST_ASSERT(gos::byte32GET(dst, i2) == 0);
                }      
            }
        }


        return 0;
    }    
}

//********************************************
namespace test7
{
    int testNetAddr_and_MacAdd()
    {
        u8  buffer[32];

        //mac address
        gos::MacAddress mac1;
        gos::MacAddress mac2;
        {

            mac1.set (1,2,3,4,5,6);
            mac2 = mac1;
            TEST_ASSERT(&mac1!=&mac2);
            TEST_ASSERT(mac1==mac2);
            TEST_FAIL_IF(mac1!=mac2);

            mac1.set (89,28,45,27,29,1);
            TEST_ASSERT(mac1!=mac2);
            mac1.serializeToBuffer (buffer, sizeof(buffer));
            mac2.deserializeFromBuffer (buffer, sizeof(buffer));
            TEST_ASSERT(mac1==mac2);
        }

        //ip
        gos::IPv4 ip1;
        gos::IPv4 ip2;
        {

            ip1.set (238,128,238,3);
            ip2 = ip1;
            TEST_ASSERT(&ip1!=&ip2);
            TEST_ASSERT(ip1==ip2);
            TEST_FAIL_IF(ip1!=ip2);

            ip1.set (99, 73, 109, 16);
            TEST_ASSERT(ip1!=ip2);
            ip1.serializeToBuffer (buffer, sizeof(buffer));
            ip2.deserializeFromBuffer (buffer, sizeof(buffer));
            TEST_ASSERT(ip1==ip2);
        }

        {
            char s[32];
            TEST_ASSERT(gos::netaddr::findMACAddress (&mac1, &ip1));
            gos::netaddr::getMACAddressAsString (mac1, s, sizeof(s), ':');
            printf ("macaddress: %s\n", s);
            printf ("ip: %d.%d.%d.%d\n", ip1.ips[0], ip1.ips[1], ip1.ips[2], ip1.ips[3]);

            gos::netaddr::setFromMACString (mac2, s, true);
            TEST_ASSERT(mac1==mac2);
        }

        gos::NetAddr naddr1;
        {
            gos::netaddr::ipstrToIPv4  ("192.168.10.12", &ip1);
            ip2.set (192, 168, 10, 12);
            TEST_ASSERT(ip1 == ip2);

		    gos::netaddr::setIPv4 (naddr1, "192.168.10.12");
            gos::netaddr::getIPv4(naddr1, &ip2);
            TEST_ASSERT(ip1 == ip2);

            ip1.set (8, 123, 87, 11);
		    gos::netaddr::setIPv4 (naddr1, ip1);
            gos::netaddr::getIPv4(naddr1, &ip2);
		    TEST_ASSERT(ip1 == ip2);
        }

        return 0;
    }
}

//********************************************
namespace test8
{
    int run1()
    {
        TEST_ASSERT (0x579B24DF == gos::utils::crc32("a"));
        TEST_ASSERT (0xE92499B0 == gos::utils::crc32("ab"));
        TEST_ASSERT (0xD3E8C673 == gos::utils::crc32("abc"));
        TEST_ASSERT (0xFA48EE30 == gos::utils::crc32("abcd"));
        TEST_ASSERT (0x498CC5F3 == gos::utils::crc32("stack-overflow"));
        TEST_ASSERT (0x5AD53D29 == gos::utils::crc32("pippo fa la pizza"));
        TEST_ASSERT (0x785392AA == gos::utils::crc32("bella CIao"));
        return 0;
    }




    enum TestEnum
    {
        CrcVal01 = COMPILE_TIME_STR_CRC32("stack-overflow"),
    };

    int run2()
    {
        u32 test1 = COMPILE_TIME_STR_CRC32("stack-overflow");
        TEST_ASSERT(0x498CC5F3==test1);
        TEST_ASSERT(0x498CC5F3==CrcVal01);
        TEST_ASSERT(COMPILE_TIME_STR_CRC32("stack-overflow")==CrcVal01);

        if constexpr (COMPILE_TIME_STR_CRC32("pippo fa la pizza") == COMPILE_TIME_STR_CRC32("pippo fa la pizza"))
        {
            TEST_ASSERT(1);
        }
        else
        {
            TEST_ASSERT(0);
        }
        
        TEST_ASSERT(0x5AD53D29==COMPILE_TIME_STR_CRC32("pippo fa la pizza"));

        TEST_ASSERT(gos::utils::crc32("stack-overflow") == 0x498CC5F3);
        TEST_ASSERT(gos::utils::crc32("stack-overflow") == CrcVal01);
        TEST_ASSERT(gos::utils::crc32("stack-overflow") == COMPILE_TIME_STR_CRC32("stack-overflow"));
        TEST_ASSERT(gos::utils::crc32("pippo fa la pizza") == COMPILE_TIME_STR_CRC32("pippo fa la pizza"));


        switch (gos::utils::crc32("stack-overflow"))
        {
        case COMPILE_TIME_STR_CRC32("stack-overflow"):
            break;

        case COMPILE_TIME_STR_CRC32("pippo fa la pizza"):
            break;

        case COMPILE_TIME_STR_CRC32("altro caso che non trigghera"):
            break;

        default:
            TEST_ASSERT(0);
        }
        return 0;
    }    
}

//********************************************
namespace test9_eDataFormat
{
    int run()
    {
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_8bit, false, 0, 1) == eDataFormat::_1u8);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_8bit, false, 0, 2) == eDataFormat::_2u8);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_8bit, false, 0, 3) == eDataFormat::_3u8);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_8bit, false, 0, 4) == eDataFormat::_4u8);

        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_8bit, true, 0, 1) == eDataFormat::_1i8);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_8bit, true, 0, 2) == eDataFormat::_2i8);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_8bit, true, 0, 3) == eDataFormat::_3i8);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_8bit, true, 0, 4) == eDataFormat::_4i8);

        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_32bit, false, 0, 1) == eDataFormat::_1u32);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_32bit, false, 0, 2) == eDataFormat::_2u32);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_32bit, false, 0, 3) == eDataFormat::_3u32);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_32bit, false, 0, 4) == eDataFormat::_4u32);

        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_32bit, true, 0, 1) == eDataFormat::_1i32);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_32bit, true, 0, 2) == eDataFormat::_2i32);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_32bit, true, 0, 3) == eDataFormat::_3i32);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_32bit, true, 0, 4) == eDataFormat::_4i32);

        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_f32, true, 0, 1) == eDataFormat::_1f32);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_f32, true, 0, 2) == eDataFormat::_2f32);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_f32, true, 0, 3) == eDataFormat::_3f32);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_f32, true, 0, 4) == eDataFormat::_4f32);        

        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_f32, true, 2, 2) == eDataFormat::_mat2x2);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_f32, true, 3, 3) == eDataFormat::_mat3x3);
        TEST_ASSERT(gos::dataformat::build (eDataFormat_type::_f32, true, 4, 4) == eDataFormat::_mat4x4);

        for (u8 isSigned=0; isSigned<2; isSigned++)
        {
            bool bSigned = false;
            if (isSigned)
                bSigned = true;

            for (u8 basicFmt=0; basicFmt<4; basicFmt++)
            {
                eDataFormat_type dataFormatType = eDataFormat_type::_8bit;
                switch (basicFmt)
                {
                default:    dataFormatType = eDataFormat_type::_8bit; break;
                case 1:     dataFormatType = eDataFormat_type::_16bit; break;
                case 2:     dataFormatType = eDataFormat_type::_32bit; break;
                case 3:     dataFormatType = eDataFormat_type::_f32; break;
                }

                for (u8 row=1; row<=4; row++)
                {
                    eDataFormat fmt = gos::dataformat::build (dataFormatType, bSigned, 0, row);
                    TEST_ASSERT(!gos::dataformat::isMatrix(fmt));
                    TEST_ASSERT(gos::dataformat::isArray(fmt));
                    TEST_ASSERT(!gos::dataformat::isArrayUNORM(fmt));
                    TEST_ASSERT(gos::dataformat::getArrayNumElem(fmt) == row);
                    TEST_ASSERT(gos::dataformat::getBasicType(fmt) == dataFormatType);
                    TEST_ASSERT(gos::dataformat::isSigned(fmt) == bSigned);

                    for (u8 col=1; col<=4; col++)
                    {
                        eDataFormat fmt = gos::dataformat::build (dataFormatType, bSigned, row, col);
                        TEST_ASSERT(gos::dataformat::isMatrix(fmt));
                        TEST_ASSERT(!gos::dataformat::isArray(fmt));
                        TEST_ASSERT(!gos::dataformat::isArrayUNORM(fmt));
                        TEST_ASSERT(gos::dataformat::getMatrixNumRow(fmt) == row);
                        TEST_ASSERT(gos::dataformat::getMatrixNumCol(fmt) == col);
                        TEST_ASSERT(gos::dataformat::getBasicType(fmt) == dataFormatType);
                        TEST_ASSERT(gos::dataformat::isSigned(fmt) == bSigned);
                    }
                }
            }
        }

        return 0;
    }    
}

//********************************************
namespace test10_eImageFormat
{
    int run()
    {
        eImageFormat fmt;

#define HELPER(s)   {\
        TEST_ASSERT(gos::utils::stringToEnum (#s, &fmt));\
        TEST_ASSERT(fmt == eImageFormat::s);\
    }\

        HELPER(U8_RGBA_sRGB)
        HELPER(U8_RGBA)
		HELPER(U8_RGB)
		HELPER(U8_R)

		HELPER(U16_RGBA)
		HELPER(U16_RGB)
		HELPER(U16_R)

		HELPER(U32_RGBA)
		HELPER(U32_RGB)
		HELPER(U32_R)

		HELPER(F32_RGBA)
		HELPER(F32_RGB)
		HELPER(F32_R)

		HELPER(U8_BGRA_sRGB)

		//depth format	(range 0xE0 - 0xEF)
    	HELPER(DEPTH_F32)
		HELPER(DEPTH_U16)
    	HELPER(DEPTH_F32_STENCIL_U8)
    	HELPER(DEPTH_U16_STENCIL_U8)
    	HELPER(DEPTH_U24_STENCIL_U8)

		//compressed format
		HELPER(DDS_BC3)
		HELPER(DDS_BC4)
		HELPER(DDS_BC5)

#undef HELPER        
        return 0;
    }
}

//********************************************
namespace test11_enum_bitmask
{
    enum class eProva : u8
    {
        none = 0,
        bit0 = 0x01,
        bit1 = 0x02,
        bit2 = 0x04,
        bit3 = 0x08,
        bit4 = 0x10,
        bit5 = 0x20,
        bit6 = 0x40,
        bit7 = 0x80
    };

    GOS_DECL_ENUM_BITMASK_CLASS(eProva);
    
    bool fn1 (eProvaBitmask bm)
    {
        return bm.isset(eProva::bit0);
    }


    int run()
    {
        eProvaBitmask bm;
        bm = eProva::none;      TEST_ASSERT(bm.bitmask == 0);
        bm = eProva::bit0;      TEST_ASSERT(bm.bitmask == 0x01);
        bm |= eProva::bit1;     TEST_ASSERT(bm.bitmask == 0x03);

        bm.zero();             TEST_ASSERT(bm.bitmask == 0);
        bm = eProva::bit4 | eProva::bit0;   TEST_ASSERT(bm.bitmask == 0x11);
        bm = bm | eProva::bit7; TEST_ASSERT(bm.bitmask == 0x91);

        eProva p;
        u8 mask = 0;
        u8 iter;
        bm.beginFetch(&iter);
        while (bm.fetch(iter, &p))
        {
            mask |= (u8)p;
        }
        TEST_ASSERT(mask == bm.bitmask);
        
        TEST_ASSERT( fn1(bm) == true );
        bm.bitclear (eProva::bit0);
        TEST_ASSERT( fn1(bm) == false );


        return 0;
    }
}



} //namespace test_gos

//********************************+
void testGos (Tester &tester)
{
    tester.run("test1 gos::system info", test_gos::test1::test_printSystemInfo);
    tester.run("test_assertion_helpers", test_gos::test_assertion_helpers::run);
    tester.run("test2 gos::handle", test_gos::test2::run);
    tester.run("test3 gos::handle array", test_gos::test3::testHandleArray);
    tester.run("test4 gos::testFS", test_gos::test4::testFS);
    tester.run("test5 gos::testStringList", test_gos::test5::testStringList);
    tester.run("test6 gos::testBitUtils", test_gos::test6::testBitUtils);
    tester.run("test7 gos::testNetAddr_and_MacAdd", test_gos::test7::testNetAddr_and_MacAdd);
    tester.run("test8 string hash(1)", test_gos::test8::run1);
    tester.run("test8 string hash(2)", test_gos::test8::run2);
    tester.run("test9 eDataFormat", test_gos::test9_eDataFormat::run);
    tester.run("test10 eImageFormat", test_gos::test10_eImageFormat::run);
    tester.run("test11 enum-bitmask", test_gos::test11_enum_bitmask::run);
}