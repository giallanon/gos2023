#include "TTest.h"
#include "gosDataBlob.h"
#include "gosMath.h"

namespace test_datablob
{



namespace test1
{
    int run()
    {
        gos::UTF8String out;
        out.prealloc (1024);


        //def builder
        u8  def1[128];
        u8  def2[128];
        u8  def3[128];
        u8  def4[512];
        u8  def5[512];
        {
            gos::datablob::DefBuilder def;

            TEST_ASSERT (false == def.isValid());
            def.begin()
                .add_simpleType ("s1", eDataFormat::_1f32)
                .end();
            TEST_ASSERT (true == def.isValid());
            TEST_ASSERT (def.getDataBlobDefSize() == 24);
            TEST_ASSERT (def.memcpyDataBlobDef (def1, sizeof(def1)));

            def.begin();
            TEST_ASSERT (false == def.isValid());
            def
                .add_simpleType ("s2", eDataFormat::_1u32)
                .add_simpleType ("s3", eDataFormat::_2u32)
                .add_simpleType ("s4", eDataFormat::_3u32)
                .add_simpleType ("s5", eDataFormat::_4u32)
                .end();
            TEST_ASSERT (true == def.isValid());
            TEST_ASSERT (def.getDataBlobDefSize() == 72);
            TEST_ASSERT (def.memcpyDataBlobDef (def2, sizeof(def2)));

            def.begin();
            TEST_ASSERT (false == def.isValid());
            def
                .struct_begin ("struct1")
                    .add_simpleType ("m1", eDataFormat::_2u8)
                    .add_simpleType ("m2", eDataFormat::_1i8)
                    .add_simpleType ("m3", eDataFormat::_3f32)
                .struct_end()
            .end();
            TEST_ASSERT (true == def.isValid());
            TEST_ASSERT (def.memcpyDataBlobDef (def3, sizeof(def3)));

            //def4
            def.begin();
            TEST_ASSERT (false == def.isValid());
            def
                .add_simpleType ("s1", eDataFormat::_2f32)
                .struct_begin ("struct1")
                    .add_simpleType ("m1", eDataFormat::_2u8)
                    .add_simpleType ("m2", eDataFormat::_1i8)
                    .add_simpleType ("m3", eDataFormat::_3f32)
                    .add_simpleType ("m4", eDataFormat::_1i8)
                .struct_end()
                .add_simpleType ("s2", eDataFormat::_1i32)
                .struct_begin ("struct2")
                    .add_simpleType ("m2_1", eDataFormat::_1u8)
                    .add_simpleType ("m2_2", eDataFormat::_1f32)
                    .add_simpleType ("m2_3", eDataFormat::_1f32)
                .struct_end()

                .array_begin1D ("array1", 32)
                    .add_simpleType (NULL, eDataFormat::_1u8)
                .array_end()
                .add_simpleType ("s3", eDataFormat::_1f32)

                .array_begin1D ("array2", 16)
                    .add_simpleType ("m3_1", eDataFormat::_1f32)
                    .add_simpleType ("m3_2", eDataFormat::_1u8)
                .array_end()
                .add_simpleType ("s4", eDataFormat::_1f32)

            .end();
            TEST_ASSERT (true == def.isValid());
            TEST_ASSERT (def.memcpyDataBlobDef (def4, sizeof(def4)));


            //def5
            def.begin();
            TEST_ASSERT (false == def.isValid());
            def
                .struct_begin ("struct1")
                    .add_simpleType ("m1", eDataFormat::_2u8)

                    .array_begin2D ("arr1", 2, 8)
                        .add_simpleType ("", eDataFormat::_1f32)
                    .array_end()

                    .array_begin2D ("arr2", 3, 4)
                        .add_simpleType ("m3_1", eDataFormat::_1u8)
                        .add_simpleType ("m3_2", eDataFormat::_1i8)
                        .array_begin1D ("m3_3_arr1", 4)
                            .add_simpleType ("", eDataFormat::_1f32)
                        .array_end()
                    .array_end()
                    
                    .add_simpleType ("m4", eDataFormat::_3f32)
                .struct_end()
            .end();
            TEST_ASSERT (true == def.isValid());
            TEST_ASSERT (def.memcpyDataBlobDef (def5, sizeof(def5)));            
        }

        //DefReader
        {
            u32 nElem;
            gos::datablob::DefReader r;
            gos::datablob::DefElem elem;
            u16     offset;

            out << "def1\n";
            gos::datablob::blobDef_prinfInfo (out, def1);
            TEST_ASSERT (r.setup (def1));
            r.beginEnumerate (&elem);
            nElem = 0;
            do
            {
                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (0 == elem.getOffset());
                TEST_ASSERT (4 == elem.getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s1", elem.getName()));
                TEST_ASSERT (eDataFormat::_1f32 == elem.getDataFmt());
                nElem++;
            } while (elem.next());
            TEST_ASSERT(1 == nElem);
            TEST_ASSERT (false == r.getOffset ("", &offset));
            TEST_ASSERT (false == r.getOffset ("pippo", &offset));
            TEST_ASSERT (false == r.getOffset ("pippo.pluto", &offset));
            TEST_ASSERT (false == r.getOffset ("s1.pluto", &offset));
            TEST_ASSERT (r.getOffset ("s1", &offset));
            TEST_ASSERT (offset == 0);


            out << "def2\n";
            gos::datablob::blobDef_prinfInfo (out, def2);
            TEST_ASSERT (r.setup (def2));
            r.beginEnumerate (&elem);
            nElem = 0;
                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (0 == elem.getOffset());
                TEST_ASSERT (4 == elem.getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s2", elem.getName()));
                TEST_ASSERT (eDataFormat::_1u32 == elem.getDataFmt());
                TEST_ASSERT (elem.next());
                nElem++;
            
                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (4 == elem.getOffset());
                TEST_ASSERT (8 == elem.getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s3", elem.getName()));
                TEST_ASSERT (eDataFormat::_2u32 == elem.getDataFmt());
                TEST_ASSERT (elem.next());
                nElem++;

                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (12 == elem.getOffset());
                TEST_ASSERT (12 == elem.getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s4", elem.getName()));
                TEST_ASSERT (eDataFormat::_3u32 == elem.getDataFmt());
                TEST_ASSERT (elem.next());
                nElem++;

                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (24 == elem.getOffset());
                TEST_ASSERT (16 == elem.getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s5", elem.getName()));
                TEST_ASSERT (eDataFormat::_4u32 == elem.getDataFmt());
                TEST_ASSERT (false == elem.next());
                nElem++;                                            
            TEST_ASSERT(4 == nElem);
            TEST_ASSERT (false == r.getOffset ("pippo", &offset));
            TEST_ASSERT (false == r.getOffset ("pippo.pluto", &offset));
            TEST_ASSERT (false == r.getOffset ("s1.pluto", &offset));
            TEST_ASSERT (r.getOffset ("s2", &offset));
            TEST_ASSERT (offset == 0);
            TEST_ASSERT (r.getOffset ("s3", &offset));
            TEST_ASSERT (offset == 4);
            TEST_ASSERT (r.getOffset ("s4", &offset));
            TEST_ASSERT (offset == 12);
            TEST_ASSERT (r.getOffset ("s5", &offset));
            TEST_ASSERT (offset == 24);

            out << "def3\n";
            gos::datablob::blobDef_prinfInfo (out, def3);
            TEST_ASSERT (r.setup (def3));
            r.beginEnumerate (&elem);
                TEST_ASSERT (15 == r.dataBlob_getSize());
                TEST_ASSERT (eDataBlobElemType::structType == elem.getType());
                TEST_ASSERT (3 == elem.structType_getNumMembers());
                TEST_ASSERT (false == elem.next());
            TEST_ASSERT (false == r.getOffset ("m1", &offset));
            TEST_ASSERT (false == r.getOffset ("m2", &offset));
            TEST_ASSERT (false == r.getOffset ("m3", &offset));
            TEST_ASSERT (false == r.getOffset ("m4", &offset));
            TEST_ASSERT (false == r.getOffset ("struct1.m4", &offset));
            TEST_ASSERT (r.getOffset ("struct1.m1", &offset));
            TEST_ASSERT (offset == 0);
            TEST_ASSERT (r.getOffset ("struct1.m2", &offset));
            TEST_ASSERT (offset == 2);
            TEST_ASSERT (r.getOffset ("struct1.m3", &offset));
            TEST_ASSERT (offset == 3);
            TEST_ASSERT (false == r.getOffset ("struct1.m2.pippo", &offset));

            out << "def4\n";
            gos::datablob::blobDef_prinfInfo (out, def4);
            TEST_ASSERT (r.setup (def4));
            r.beginEnumerate (&elem);
                TEST_ASSERT (157 == r.dataBlob_getSize());
                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (elem.next());
                
                TEST_ASSERT (eDataBlobElemType::structType == elem.getType());
                TEST_ASSERT (4 == elem.structType_getNumMembers());
                TEST_ASSERT (elem.next());

                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (elem.next());

                TEST_ASSERT (eDataBlobElemType::structType == elem.getType());
                TEST_ASSERT (3 == elem.structType_getNumMembers());
                TEST_ASSERT (elem.next());
            TEST_ASSERT (false == r.getOffset ("m1", &offset));
            TEST_ASSERT (false == r.getOffset ("m2", &offset));
            TEST_ASSERT (false == r.getOffset ("m3", &offset));
            TEST_ASSERT (false == r.getOffset ("m4", &offset));
            
            TEST_ASSERT (r.getOffset ("struct1.m1", &offset));      TEST_ASSERT (offset == 8);
            TEST_ASSERT (r.getOffset ("struct1.m2", &offset));      TEST_ASSERT (offset == 10);
            TEST_ASSERT (r.getOffset ("struct1.m3", &offset));      TEST_ASSERT (offset == 11);
            TEST_ASSERT (r.getOffset ("struct1.m4", &offset));      TEST_ASSERT (offset == 23);
            TEST_ASSERT (false == r.getOffset ("struct1.m5", &offset));
            TEST_ASSERT (false == r.getOffset ("struct1.m2.pippo", &offset));

            TEST_ASSERT (r.getOffset ("s1", &offset));      TEST_ASSERT (offset == 0);
            TEST_ASSERT (r.getOffset ("s2", &offset));      TEST_ASSERT (offset == 24);
            TEST_ASSERT (r.getOffset ("s3", &offset));      TEST_ASSERT (offset == 69);
            TEST_ASSERT (r.getOffset ("s4", &offset));      TEST_ASSERT (offset == 153);

            TEST_ASSERT (r.getOffset ("struct2.m2_1", &offset));      TEST_ASSERT (offset == 28);
            TEST_ASSERT (r.getOffset ("struct2.m2_2", &offset));      TEST_ASSERT (offset == 29);
            TEST_ASSERT (r.getOffset ("struct2.m2_3", &offset));      TEST_ASSERT (offset == 33);

            TEST_ASSERT (r.getOffset ("array1", &offset));      TEST_ASSERT (offset == 37);
            TEST_ASSERT (r.getOffset ("array1[0]", &offset));      TEST_ASSERT (offset == 37);
            TEST_ASSERT (r.getOffset ("array1[1]", &offset));      TEST_ASSERT (offset == 38);
            TEST_ASSERT (r.getOffset ("array1[31]", &offset));      TEST_ASSERT (offset == 68);
            TEST_ASSERT (false == r.getOffset ("array1[32]", &offset));
            TEST_ASSERT (false == r.getOffset ("array1[0][1]", &offset));

            TEST_ASSERT (r.getOffset ("array2.m3_1", &offset));                  TEST_ASSERT (offset == 73);
            TEST_ASSERT (r.getOffset ("array2[0].m3_1", &offset));               TEST_ASSERT (offset == 73);
            TEST_ASSERT (r.getOffset ("array2[0].m3_2", &offset));               TEST_ASSERT (offset == 77);
            TEST_ASSERT (r.getOffset ("array2[1].m3_1", &offset));               TEST_ASSERT (offset == 78);
            TEST_ASSERT (r.getOffset ("array2[1].m3_2", &offset));               TEST_ASSERT (offset == 82);
            TEST_ASSERT (r.getOffset ("array2[2].m3_1", &offset));               TEST_ASSERT (offset == 83);
            TEST_ASSERT (r.getOffset ("array2[2].m3_2", &offset));               TEST_ASSERT (offset == 87);
            TEST_ASSERT (r.getOffset ("array2[15].m3_1", &offset));              TEST_ASSERT (offset == 148);
            TEST_ASSERT (r.getOffset ("array2[15].m3_2", &offset));              TEST_ASSERT (offset == 152);
            TEST_ASSERT (false == r.getOffset ("array2[16]", &offset));
            TEST_ASSERT (false == r.getOffset ("array2[16].m3_1", &offset));
            TEST_ASSERT (false == r.getOffset ("array2[16].m3_2", &offset));
            TEST_ASSERT (false == r.getOffset ("array2.pippo", &offset));
            TEST_ASSERT (false == r.getOffset ("array2[0].pippo", &offset));
            TEST_ASSERT (false == r.getOffset ("array2[4].m3_1.pippo", &offset));



            out << "def5\n";
            TEST_ASSERT (r.setup (def5));
            gos::datablob::blobDef_prinfInfo (out, def5, [](gos::UTF8String &out, const gos::datablob::DefElem &elem) {
            
                out << gos::STRFMT("0x%08X", elem.getUserData());
            });
            printf ("%s\n", out.getBuffer());
            for (u8 i1=0; i1<2; i1++)
            {
                for (u8 i2=0; i2<8; i2++)
                {
                    char s[64];
                    sprintf_s (s, sizeof(s), "struct1.arr1[%d][%d]", i1, i2);
                    TEST_ASSERT(r.getOffset(s, &offset));
                    TEST_ASSERT(offset == 2 + 4*(i1*8 +i2));
                }
            }
            TEST_ASSERT(r.getOffset("struct1.arr1", &offset));  TEST_ASSERT(offset == 2);
            TEST_ASSERT(false == r.getOffset("struct1.arr1[0]", &offset));
            TEST_ASSERT(false == r.getOffset("struct1.arr1[0][9]", &offset));
            TEST_ASSERT(false == r.getOffset("struct1.arr1[2][0]", &offset));


            TEST_ASSERT(r.getOffset("struct1.arr2", &offset));  TEST_ASSERT(offset == 66);
            TEST_ASSERT(r.getOffset("struct1.arr2[0][0].m3_1", &offset));  TEST_ASSERT(offset == 66);
            TEST_ASSERT(r.getOffset("struct1.arr2[0][0].m3_2", &offset));  TEST_ASSERT(offset == 67);
            TEST_ASSERT(false == r.getOffset("struct1.arr2[0][0].pippo", &offset));
            for (u8 i1=0; i1<3; i1++)
            {
                for (u8 i2=0; i2<4; i2++)
                {
                    char s[64];
                    sprintf_s (s, sizeof(s), "struct1.arr2[%d][%d]", i1, i2);
                    TEST_ASSERT(r.getOffset(s, &offset));
                    TEST_ASSERT(offset == 66 + 18*(i1*4 +i2));
                }
            }
            
            TEST_ASSERT(r.getOffset("struct1.arr2[0][0].m3_1", &offset));           TEST_ASSERT(offset == 66);
            TEST_ASSERT(r.getOffset("struct1.arr2[0][0].m3_3_arr1", &offset));      TEST_ASSERT(offset == 68);
            TEST_ASSERT(r.getOffset("struct1.arr2[0][0].m3_3_arr1[0]", &offset));   TEST_ASSERT(offset == 68);
            TEST_ASSERT(r.getOffset("struct1.arr2[0][0].m3_3_arr1[1]", &offset));   TEST_ASSERT(offset == 72);
            TEST_ASSERT(r.getOffset("struct1.arr2[0][0].m3_3_arr1[2]", &offset));   TEST_ASSERT(offset == 76);
            TEST_ASSERT(r.getOffset("struct1.arr2[0][0].m3_3_arr1[3]", &offset));   TEST_ASSERT(offset == 80);
            TEST_ASSERT(false == r.getOffset("struct1.arr2[0][0].m3_3_arr1[4]", &offset));

            TEST_ASSERT(r.getOffset("struct1.arr2[0][1].m3_1", &offset));           TEST_ASSERT(offset == 66 + 18);
            TEST_ASSERT(r.getOffset("struct1.arr2[0][1].m3_3_arr1[0]", &offset));   TEST_ASSERT(offset == 66 + 18 +2);
            TEST_ASSERT(r.getOffset("struct1.arr2[0][1].m3_3_arr1[1]", &offset));   TEST_ASSERT(offset == 66 + 18 +2 +4);
            TEST_ASSERT(r.getOffset("struct1.arr2[0][1].m3_3_arr1[2]", &offset));   TEST_ASSERT(offset == 66 + 18 +2 +8);
            TEST_ASSERT(r.getOffset("struct1.arr2[0][1].m3_3_arr1[3]", &offset));   TEST_ASSERT(offset == 66 + 18 +2 +12);
            
            TEST_ASSERT(r.getOffset("struct1.arr2[1][0].m3_1", &offset));           TEST_ASSERT(offset == 66 + 18*4);
            TEST_ASSERT(r.getOffset("struct1.arr2[1][0].m3_3_arr1[0]", &offset));   TEST_ASSERT(offset == 66 + 18*4 +2);
            TEST_ASSERT(r.getOffset("struct1.arr2[1][0].m3_3_arr1[1]", &offset));   TEST_ASSERT(offset == 66 + 18*4 +2 +4);
            TEST_ASSERT(r.getOffset("struct1.arr2[1][0].m3_3_arr1[2]", &offset));   TEST_ASSERT(offset == 66 + 18*4 +2 +8);
            TEST_ASSERT(r.getOffset("struct1.arr2[1][0].m3_3_arr1[3]", &offset));   TEST_ASSERT(offset == 66 + 18*4 +2 +12);

            TEST_ASSERT(r.getOffset("struct1.arr2[1][2].m3_1", &offset));           TEST_ASSERT(offset == 66 + 18*6);
            TEST_ASSERT(r.getOffset("struct1.arr2[1][2].m3_3_arr1[0]", &offset));   TEST_ASSERT(offset == 66 + 18*6 +2);
            TEST_ASSERT(r.getOffset("struct1.arr2[1][2].m3_3_arr1[1]", &offset));   TEST_ASSERT(offset == 66 + 18*6 +2 +4);
            TEST_ASSERT(r.getOffset("struct1.arr2[1][2].m3_3_arr1[2]", &offset));   TEST_ASSERT(offset == 66 + 18*6 +2 +8);
            TEST_ASSERT(r.getOffset("struct1.arr2[1][2].m3_3_arr1[3]", &offset));   TEST_ASSERT(offset == 66 + 18*6 +2 +12);
        }

        return 0;
    }
} //namespace test1




namespace test2
{
    int test1(gos::datablob::Var &v)
    {
        v.zero();

        v.set ("f1", 1.2f);
        v.set ("f2", gos::vec2f(3.4f, 4.5f));
        v.set ("f3", gos::vec3f(5.6f, 6.7f, 7.8f));
        
        v.set<u8> ("u1", 32);
        v.set ("u2", gos::vec2u8(33, 34));
        
        v.set<i32> ("i1", -1000);
        v.set ("i2", gos::vec2i(-2000, -3000));
        v.set ("i3", gos::vec3i(-2000, -3000, -4000));

        TEST_ASSERT (v.get<f32>("f1") == 1.2f);
        TEST_ASSERT (v.get<gos::vec2f>("f2") == gos::vec2f(3.4f, 4.5f));
        TEST_ASSERT (v.get<gos::vec3f>("f3") == gos::vec3f(5.6f, 6.7f, 7.8f));
        
        TEST_ASSERT (v.get<u8>("u1") == 32);
        TEST_ASSERT (v.get<gos::vec2u8>("u2") == gos::vec2u8(33, 34));
        
        TEST_ASSERT (v.get<i32>("i1") == -1000);
        TEST_ASSERT (v.get<gos::vec2i>("i2") == gos::vec2i(-2000, -3000));
        TEST_ASSERT (v.get<gos::vec3i>("i3") == gos::vec3i(-2000, -3000, -4000));
        
        return 0;
    }

    int run()
    {
        gos::Allocator *localAllocator = gos::getSysHeapAllocator();
        u8  *dataBlobDef = NULL;
        u8  *dataBlob = NULL;
        
        {
            gos::datablob::DefBuilder def;
            def.begin()
                .add_simpleType ("f1", eDataFormat::_1f32)
                .add_simpleType ("f2", eDataFormat::_2f32)
                .add_simpleType ("f3", eDataFormat::_3f32)
                
                .add_simpleType ("u1", eDataFormat::_1u8)
                .add_simpleType ("u2", eDataFormat::_2u8)
                
                .add_simpleType ("i1", eDataFormat::_1i32)
                .add_simpleType ("i2", eDataFormat::_2i32)
                .add_simpleType ("i3", eDataFormat::_3i32)
            .end();
            dataBlobDef = def.allocDataBlobDef (localAllocator);
        }

        gos::datablob::DefReader reader;
        reader.setup (dataBlobDef);

        dataBlob = gos::datablob::createNew (localAllocator, dataBlobDef);
        
        gos::datablob::Var v1;
        v1.setup (&reader, dataBlob);
        
        int ret = test1(v1);
        GOSFREE(localAllocator, dataBlobDef);
        GOSFREE(localAllocator, dataBlob);
        
        return ret;
    }
} //namespace test2


} //namespace test_datablob

//********************************+
void testDataBlob (Tester &tester)
{
    tester.run("test_datablob::test1", test_datablob::test1::run);
    tester.run("test_datablob::test2", test_datablob::test2::run);
}