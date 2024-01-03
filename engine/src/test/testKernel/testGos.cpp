#include "TTest.h"
#include "gosHandle.h"
#include "string/gosStringList.h"
#include "gosUtils.h"

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


            //verifico che la risoluzione dei path funzioni
            //Essendo nella cartella "writable", posso usare il carattere speciale @ per indicare quella cartella
            gos::string::utf8::spf (s2, sizeof(s2), "@/testFS/dir1/file_di_esempio%02d.txt", i);
            TEST_ASSERT(gos::fs::fileExists(s2));
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
        
        

    return 0;
    }

    //********************************************
    int testStringList()
    {
        gos::Allocator *allocator = gos::getSysHeapAllocator();
        gos::StringList sl;

        sl.setup (allocator, 1024);
        sl.add ("Pippo");    
        sl.add ("Pluto");
        sl.add ("paperino");

        TEST_ASSERT(sl.getNumString() == 3);

        return 0;
    }

    //********************************************
    int testBitUtils()
    {
        {
            static const u8     BUFFER_SIZE_BYTE = 32;
            static const u32    NUM_BIT = BUFFER_SIZE_BYTE * 8;
            u8 dst[BUFFER_SIZE_BYTE];

            gos::utils::bitZERO (dst, sizeof(dst));
            for (u32 i=0; i<BUFFER_SIZE_BYTE; i++)
            {
                TEST_ASSERT(dst[i] == 0);
            }

            for (u32 i=0; i<NUM_BIT; i++)
            {
                //setto 1 bit alla volta
                gos::utils::bitSET (dst, BUFFER_SIZE_BYTE, i);

                //verifico che tutti i primi [i] bit siano a 1
                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(gos::utils::isBitSET(dst, BUFFER_SIZE_BYTE, i2));
                }
                
                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BIT; i2++)
                {
                    TEST_ASSERT(!gos::utils::isBitSET(dst, BUFFER_SIZE_BYTE, i2));
                }            
            }
        }

        //ripeto quanto fatto sopra ma per il caso u32
        {
            static const u8 NUM_BIT = 32;
            
            u32 dst;
            gos::utils::bitZERO (&dst);
            TEST_ASSERT(dst == 0);

            for (u32 i=0; i<NUM_BIT; i++)
            {
                //setto 1 bit alla volta
                gos::utils::bitSET (&dst, i);

                //verifico che tutti i primi [i] bit siano a 1
                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(gos::utils::isBitSET(&dst, i2));
                }
                
                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BIT; i2++)
                {
                    TEST_ASSERT(!gos::utils::isBitSET(&dst, i2));
                }            
            }
        }
        

        //ripeto quanto fatto sopra ma per il caso u8
        {
            static const u8 NUM_BIT = 8;
            
            u8 dst;
            gos::utils::bitZERO (&dst);
            TEST_ASSERT(dst == 0);

            for (u32 i=0; i<NUM_BIT; i++)
            {
                //setto 1 bit alla volta
                gos::utils::bitSET (&dst, i);

                //verifico che tutti i primi [i] bit siano a 1
                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(gos::utils::isBitSET(&dst, i2));
                }
                
                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BIT; i2++)
                {
                    TEST_ASSERT(!gos::utils::isBitSET(&dst, i2));
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
                gos::utils::byteSET (&dst, byteValueList[i], i);

                for (u32 i2=0; i2<(i+1); i2++)
                {
                    TEST_ASSERT(gos::utils::byteGET(&dst, i2) == byteValueList[i2]);
                }
                
                //e che tutti i successivi siano a 0
                for (u32 i2=(i+1); i2<NUM_BYTE; i2++)
                {
                    TEST_ASSERT(gos::utils::byteGET(&dst, i2) == 0);
                }      
            }
        }


        return 0;
    }    
    
    //********************************************
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
} //namespace test_gos

//********************************+
void testGos (Tester &tester)
{
    tester.run("gos::system info", test_gos::test_printSystemInfo);
    tester.run("gos::handle", test_gos::testHandle);
    tester.run("gos::handle array", test_gos::testHandleArray, gos::getSysHeapAllocator());
    tester.run("gos::testFS", test_gos::testFS);
    tester.run("gos::testStringList", test_gos::testStringList);
    tester.run("gos::testBitUtils", test_gos::testBitUtils);
    tester.run("gos::testNetAddr_and_MacAdd", test_gos::testNetAddr_and_MacAdd);
}