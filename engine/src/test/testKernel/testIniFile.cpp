#include "TTest.h"
#include "gosIniFile.h"

namespace test_iniFile
{


//*******************************
namespace testJSON
{
    int loadJsonParseAndSave (const char *jsonFilename)
    {
        u32 fsize;
        u8 *buffer = gos::fs::fileLoadInMemory (gos::getSysHeapAllocator(), jsonFilename, &fsize);
        TEST_ASSERT(NULL != buffer);

        gos::IniFile ini;
        TEST_ASSERT(ini.fromJSon (buffer, fsize));

        char s[1024];
        sprintf_s (s, sizeof(s), "%s.CONVERTED.txt", jsonFilename);
        ini.saveAs (s);

        GOSFREE(gos::getSysHeapAllocator(), buffer);

        return 0;     
    }

    int run()
    {
        
        TEST_FAIL_IF(0 != loadJsonParseAndSave("@w/testIniFile/json_complicato.txt"));

        TEST_FAIL_IF(0 != loadJsonParseAndSave("@w/testIniFile/json_complicato2.txt"));
        {
            gos::IniFile ini;
            TEST_ASSERT(ini.loadAndParse("@w/testIniFile/json_complicato2.txt.CONVERTED.txt"));
            TEST_ASSERT(5126 == ini.getOrDefaultAsU32 ("accessors[0].componentType", u32MAX));
            TEST_ASSERT(24 == ini.getOrDefaultAsU32   ("accessors[0].count", u32MAX));
            TEST_ASSERT(1 == ini.getOrDefaultAsU32    ("accessors[0].max[0]", u32MAX));
            TEST_ASSERT(ini.checkString               ("accessors[0].max[1]", "marco"));
            TEST_ASSERT(3 == ini.getOrDefaultAsU32    ("accessors[0].max[2]", u32MAX));
            TEST_ASSERT(-1 == ini.getOrDefaultAsI32   ("accessors[0].min[0]", 0));
            TEST_ASSERT(-2 == ini.getOrDefaultAsI32   ("accessors[0].min[1]", 0));
            TEST_ASSERT(-3 == ini.getOrDefaultAsI32   ("accessors[0].min[2]", 0));

            TEST_ASSERT(43 == ini.getOrDefaultAsU32 ("accessors[1].componentType", u32MAX));
            TEST_ASSERT(89 == ini.getOrDefaultAsU32 ("accessors[1].count", u32MAX));
        }

        TEST_FAIL_IF(0 != loadJsonParseAndSave("@w/testIniFile/json_complicato3.txt"));
        {
            gos::IniFile ini;
            TEST_ASSERT(ini.loadAndParse("@w/testIniFile/json_complicato3.txt.CONVERTED.txt"));
            TEST_ASSERT(0 == ini.getOrDefaultAsU32 ("scenes[0].nodes[0]", u32MAX));
            TEST_ASSERT(1 == ini.getOrDefaultAsU32 ("textures[1].source", u32MAX));
            TEST_ASSERT(6 == ini.getOrDefaultAsU32 ("images[1].bufferView", u32MAX));
            TEST_ASSERT(3753342 == ini.getOrDefaultAsU32 ("bufferViews[5].byteLength", u32MAX));
            TEST_ASSERT(-14 == ini.getOrDefaultAsI32 ("accessors[0].max[2]", u32MAX));
            TEST_ASSERT(1 == ini.getOrDefaultAsU32 ("meshes[0].primitives[0].attributes.NORMAL", u32MAX));
            TEST_ASSERT(2 == ini.getOrDefaultAsU32 ("meshes[0].primitives[0].attributes.TEXCOORD_0", u32MAX));

            TEST_ASSERT(NULL != ini.getSubsection ("scenes[0]"));
            TEST_ASSERT(NULL == ini.getSubsection ("scenes[1]"));
            TEST_ASSERT(NULL != ini.getSubsection ("meshes[0].primitives[0]"));

            gos::IniFileSection *a = ini.getSubsection ("textures[0]");
            TEST_ASSERT(NULL != a);

            gos::IniFileSection *b = ini.getSubsection ("textures[1]");
            TEST_ASSERT(NULL != b);

            TEST_ASSERT(a != b);
        }

        TEST_FAIL_IF(0 != loadJsonParseAndSave("@w/testIniFile/test2.json"));
        {
            gos::IniFile ini;
            TEST_ASSERT(ini.loadAndParse("@w/testIniFile/test2.json.CONVERTED.txt"));
            TEST_ASSERT(0 == ini.getOrDefaultAsU32 ("scenes[0].nodes[0]", u32MAX));
            TEST_ASSERT(1 == ini.getOrDefaultAsU32 ("scenes[0].nodes[1]", u32MAX));
            TEST_ASSERT(0 == ini.getOrDefaultAsU32 ("nodes[0].mesh", u32MAX));
            TEST_ASSERT(0 == ini.getOrDefaultAsF32 ("nodes[0].rotation[0]", -10000.0f));
            TEST_ASSERT(.015707f == ini.getOrDefaultAsF32 ("nodes[0].rotation[1]", -10000.0f));
            TEST_ASSERT(0 == ini.getOrDefaultAsF32 ("nodes[0].rotation[2]", -10000.0f));
            TEST_ASSERT(0.999877f == ini.getOrDefaultAsF32 ("nodes[0].rotation[3]", -10000.0f));
        }  

        return 0;
    }
}


namespace test1
{
    void printSectionStartingLine (const gos::IniFileSection *sec)
    {
        printf ("line %03d\t\t%s\n", sec->getLineStarted(), sec->name.getBuffer());

        for (u32 i=0; i<sec->getNSubsection(); i++)
            printSectionStartingLine (sec->getSubsectionByIndex(i));
    }


    int checkIT (gos::IniFile &ini)
    {
        TEST_ASSERT(ini.checkString("nome_sezione1.campo1", "ciao"));
        TEST_ASSERT(ini.checkString("nome_sezione1.campo2", "hello"));
        TEST_ASSERT(ini.checkString("nome_sezione1.campo3", "amico mio"));
        TEST_ASSERT(10 == ini.getOrDefaultAsU32("nome_sezione1.campo4", 0));
        TEST_ASSERT(11 == ini.getOrDefaultAsU32("nome_sezione1.campo5", 0));
        TEST_ASSERT(-20 == ini.getOrDefaultAsI32("nome_sezione1.campo6", 0));
        TEST_ASSERT(-24 == ini.getOrDefaultAsI32("nome_sezione1.campo7", 0));
        TEST_ASSERT(1.35f == ini.getOrDefaultAsF32("nome_sezione1.campo8", 0));
        TEST_ASSERT(1.36f == ini.getOrDefaultAsF32("nome_sezione1.campo9", 0));
        TEST_ASSERT(-12.79f == ini.getOrDefaultAsF32("nome_sezione1.campo10", 0));
        TEST_ASSERT(-14.31f == ini.getOrDefaultAsF32("nome_sezione1.campo11", 0));

        TEST_ASSERT(ini.checkString("nome_sezione1.nome_sezione2.campo1", "2-ciao"));
        TEST_ASSERT(ini.checkString("nome_sezione1.nome_sezione2.campo2", "2-hello"));
        TEST_ASSERT(ini.checkString("nome_sezione1.nome_sezione2.campo3", "2-amico mio"));
        TEST_ASSERT(38 == ini.getOrDefaultAsU32("nome_sezione1.nome_sezione2.campo4", 0));
        TEST_ASSERT(66 == ini.getOrDefaultAsU32("nome_sezione1.nome_sezione2.campo5", 0));
        TEST_ASSERT(-356 == ini.getOrDefaultAsI32("nome_sezione1.nome_sezione2.campo6", 0));
        TEST_ASSERT(-86 == ini.getOrDefaultAsI32("nome_sezione1.nome_sezione2.campo7", 0));
        TEST_ASSERT(9.678f == ini.getOrDefaultAsF32("nome_sezione1.nome_sezione2.campo8", 0));
        TEST_ASSERT(88.21f == ini.getOrDefaultAsF32("nome_sezione1.nome_sezione2.campo9", 0));
        TEST_ASSERT(-128.659f == ini.getOrDefaultAsF32("nome_sezione1.nome_sezione2.campo10", 0));
        TEST_ASSERT(-1903.21f == ini.getOrDefaultAsF32("nome_sezione1.nome_sezione2.campo11", 0));

        //array
        TEST_ASSERT(8 == ini.getOrDefaultAsU32("nome_sezione1.lista[0]", 0));
        TEST_ASSERT(9 == ini.getOrDefaultAsU32("nome_sezione1.lista[1]", 0));
        TEST_ASSERT(10 == ini.getOrDefaultAsU32("nome_sezione1.lista[2]", 0));
        TEST_ASSERT(11 == ini.getOrDefaultAsU32("nome_sezione1.lista[3]", 0));

        TEST_ASSERT(ini.checkString("nome_sezione1.hobby[0]", "calcio"));
        TEST_ASSERT(ini.checkString("nome_sezione1.hobby[1]", "tennis"));

        TEST_ASSERT(ini.checkString("nome_sezione1.autore[0].nome", "mario"));
        TEST_ASSERT(91 == ini.getOrDefaultAsU32("nome_sezione1.autore[0].eta", 0));

        TEST_ASSERT(ini.checkString("nome_sezione1.autore[1].nome", "marco"));
        TEST_ASSERT(19 == ini.getOrDefaultAsU32("nome_sezione1.autore[1].eta", 0));
        TEST_ASSERT(ini.checkString("nome_sezione1.autore[1].other[0].nickname", "il segugio"));
        TEST_ASSERT(1974 == ini.getOrDefaultAsU32("nome_sezione1.autore[1].other[1].anno_nascita", 0));
        TEST_ASSERT(ini.checkString("nome_sezione1.autore[1].other[1].hobby[0]", "cucina"));
        TEST_ASSERT(ini.checkString("nome_sezione1.autore[1].other[1].hobby[1]", "pc"));

        TEST_ASSERT(ini.checkString("nome_sezione1.autore[2].nome", "giovanna"));
        TEST_ASSERT(45 == ini.getOrDefaultAsU32("nome_sezione1.autore[2].eta", 0));

        TEST_ASSERT(73983 == ini.getOrDefaultAsU32("nome_sezione1.biografia[0].rif", 0));
        TEST_ASSERT(30923 == ini.getOrDefaultAsU32("nome_sezione1.biografia[1].rif", 0));


        TEST_ASSERT(ini.checkString("nome_sezione1.nome_sezione2.hobby[0]", "sport"));
        TEST_ASSERT(ini.checkString("nome_sezione1.nome_sezione2.hobby[1]", "nave"));
        return 0;
    }

    int run()
    {
        {
            gos::IniFile ini;
            TEST_ASSERT(ini.loadAndParse("@w/testIniFile/test1.txt"));
            ini.saveAs("@w/testIniFile/test1_PARSED.txt");
            TEST_FAIL_IF(0 != checkIT(ini));

            printSectionStartingLine(ini.getRoot());
            TEST_ASSERT(ini.getSubsection("nome_sezione1")->getLineStarted() == 1);
            TEST_ASSERT(ini.getSubsection("nome_sezione1.nome_sezione2")->getLineStarted() == 25);
            TEST_ASSERT(ini.getSubsection("nome_sezione1.autore@0@")->getLineStarted() == 44);
            TEST_ASSERT(ini.getSubsection("nome_sezione1.autore@1@")->getLineStarted() == 50);
            TEST_ASSERT(ini.getSubsection("nome_sezione1.autore@1@.other@0@")->getLineStarted() == 55);
            TEST_ASSERT(ini.getSubsection("nome_sezione1.autore@1@.other@1@")->getLineStarted() == 60);
            TEST_ASSERT(ini.getSubsection("nome_sezione1.@direttiva@0@")->getLineStarted() == 69);
            TEST_ASSERT(ini.getSubsection("nome_sezione1.@direttiva2@0@")->getLineStarted() == 71);
            TEST_ASSERT(ini.getSubsection("nome_sezione1.@direttiva3@0@")->getLineStarted() == 76);
            TEST_ASSERT(ini.getSubsection("nome_sezione1.autore@2@")->getLineStarted() == 81);
            TEST_ASSERT(ini.getSubsection("nome_sezione1.biografia@0@")->getLineStarted() == 87);
            TEST_ASSERT(ini.getSubsection("nome_sezione1.biografia@1@")->getLineStarted() == 97);
            TEST_ASSERT(ini.getSubsection("nome_sezione2")->getLineStarted() == 106);
        }
        
        {
            gos::IniFile ini;
            TEST_ASSERT(ini.loadAndParse("@w/testIniFile/test1_PARSED.txt"));
            TEST_FAIL_IF(0 != checkIT(ini));
        }



        return 0;
    }
} //namespace test1


namespace test_direttive
{
    int checkDirettiva (gos::IniFile &ini, const char *nomeDirettiva, const char *valore)
    {
        const gos::IniFileSection *sec = ini.getSubsection(nomeDirettiva);
        TEST_ASSERT(NULL != sec);

        char s[1024];
        if (0x00 == valore[0])
        {
            TEST_ASSERT(false == sec->get("__value", s, sizeof(s)));
        }
        else
        {
            TEST_ASSERT(sec->get("__value", s, sizeof(s)));
            TEST_ASSERT(0 == strcmp(valore, s));
        }    

        return 0;
    }

    int checkIT (const char *fname)
    {
        char s[512];
        gos::IniFile ini;
        TEST_ASSERT(ini.loadAndParse(fname));
        
        //gos::UTF8String out;    ini.debug_print(out);   printf ("%s\n\n", out.getBuffer());

        TEST_ASSERT(13 == ini.getNSubsection());

        for (u32 i=0; i<ini.getNSubsection(); i++)
        {
            const gos::IniFileSection *sec = ini.getSubsectionByIndex(i);
            TEST_ASSERT(NULL != sec);

            const gos::IniFileSection *sec2 = ini.getSubsection (sec->name.getBuffer());
            TEST_ASSERT(NULL != sec2);
        }

        checkDirettiva(ini, "@direttivaA@0@", "");
        checkDirettiva(ini, "@direttiva1@0@", "include anche un : carattere speciale");
        checkDirettiva(ini, "@direttiva1@1@", "secondo valore");
        checkDirettiva(ini, "@direttiva1@2@", "terzo valore");
        TEST_ASSERT (3 == ini.getOrDefaultAsU32("@direttiva1@2@.__value", 0));

        checkDirettiva(ini, "@direttiva2@0@", "ciao'amico'[mio]");
        checkDirettiva(ini, "@direttiva3@0@", "prova");

        TEST_ASSERT(true == ini.get("@direttiva4@0@.__value", s, sizeof(s)));
        TEST_ASSERT(0 == strcmp(s, "questa ha anche dei valori in sezione"));
        TEST_ASSERT (10 == ini.getOrDefaultAsU32("@direttiva4@0@.m4_1", 0));
        TEST_ASSERT(true == ini.get("@direttiva4@0@.m4_2", s, sizeof(s)));
        TEST_ASSERT(0 == strcmp(s, "ciao amico"));

        return 0;
    }

    int run()
    {
        gos::IniFile ini;
        TEST_ASSERT(ini.loadAndParse("@w/testIniFile/test_direttive.txt"));
        ini.saveAs("@w/testIniFile/test_direttive_PARSED.txt");


        TEST_FAIL_IF(0 != checkIT("@w/testIniFile/test_direttive.txt"));        
        //TEST_FAIL_IF(0 != checkIT("@w/testIniFile/test_direttive_PARSED.txt"));

        return 0;
    }
} //namespace test_direttive


} //namespace test_thread


//********************************+
void testIniFile (Tester &tester)
{
    tester.run("iniFile::test1", test_iniFile::test1::run);
    tester.run("iniFile::testJSON", test_iniFile::testJSON::run);
    tester.run("iniFile::test_direttive", test_iniFile::test_direttive::run);
}