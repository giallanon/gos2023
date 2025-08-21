#include "TTest.h"
#include "gosDataBlob.h"

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

                .array_begin1D ("array1", 16)
                    .add_simpleType ("m3_1", eDataFormat::_1f32)
                    .add_simpleType ("m3_2", eDataFormat::_1u8)
                .array_end()
                .add_simpleType ("s4", eDataFormat::_1f32)

            .end();
            TEST_ASSERT (true == def.isValid());
            TEST_ASSERT (def.memcpyDataBlobDef (def4, sizeof(def4)));

            def.begin();
            TEST_ASSERT (false == def.isValid());
            def
                .struct_begin ("struct1")
                    .add_simpleType ("m1", eDataFormat::_2u8)
                    .array_begin2D ("arr1", 3, 4)
                        .add_simpleType ("m2_1", eDataFormat::_1u8)
                        .add_simpleType ("m2_2", eDataFormat::_1i8)
                    .array_end()
                    .add_simpleType ("m3", eDataFormat::_3f32)
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



            out << "def5\n";
            gos::datablob::blobDef_prinfInfo (out, def5, [](gos::UTF8String &out, const gos::datablob::DefElem &elem) {
            
                out << gos::STRFMT("0x%08X", elem.getUserData());
            });
            printf ("%s\n", out.getBuffer());
        }

        return 0;
    }    
} //namespace test1

} //namespace test_datablob

//********************************+
void testDataBlob (Tester &tester)
{
    tester.run("test_datablob::test1", test_datablob::test1::run);
}