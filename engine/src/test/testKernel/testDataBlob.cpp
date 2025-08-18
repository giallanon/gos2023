#include "TTest.h"
#include "gosDataBlob.h"

namespace test_datablob
{



namespace test1
{
    int run()
    {
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
                    .struct_begin ("struct3")
                        .add_simpleType ("m3_1", eDataFormat::_1f32)
                        .add_simpleType ("m3_2", eDataFormat::_1u8)
                    .struct_end()
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
                        .struct_begin ("struct2")
                            .add_simpleType ("m2_1", eDataFormat::_1u8)
                            .add_simpleType ("m2_2", eDataFormat::_1i8)
                        .struct_end()
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
            gos::datablob::DefReader::Elem elem;

            gos::datablob::print_info ("def1", def1);
            TEST_ASSERT (r.begin (def1, &elem));
            nElem = 0;
            do
            {
                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (0 == elem.getOffset());
                TEST_ASSERT (4 == elem.getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s1", elem.getName()));
                TEST_ASSERT (eDataFormat::_1f32 == elem.simpleType_getDataFmt());
                nElem++;
            } while (elem.next());
            TEST_ASSERT(1 == nElem);


            gos::datablob::print_info ("def2", def2);
            TEST_ASSERT (r.begin (def2, &elem));
            nElem = 0;
                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (0 == elem.getOffset());
                TEST_ASSERT (4 == elem.getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s2", elem.getName()));
                TEST_ASSERT (eDataFormat::_1u32 == elem.simpleType_getDataFmt());
                TEST_ASSERT (elem.next());
                nElem++;
            
                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (4 == elem.getOffset());
                TEST_ASSERT (8 == elem.getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s3", elem.getName()));
                TEST_ASSERT (eDataFormat::_2u32 == elem.simpleType_getDataFmt());
                TEST_ASSERT (elem.next());
                nElem++;

                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (12 == elem.getOffset());
                TEST_ASSERT (12 == elem.getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s4", elem.getName()));
                TEST_ASSERT (eDataFormat::_3u32 == elem.simpleType_getDataFmt());
                TEST_ASSERT (elem.next());
                nElem++;

                TEST_ASSERT (eDataBlobElemType::simpleType == elem.getType());
                TEST_ASSERT (24 == elem.getOffset());
                TEST_ASSERT (16 == elem.getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s5", elem.getName()));
                TEST_ASSERT (eDataFormat::_4u32 == elem.simpleType_getDataFmt());
                TEST_ASSERT (false == elem.next());
                nElem++;                                            
            TEST_ASSERT(4 == nElem);


            gos::datablob::print_info ("def3", def3);
            TEST_ASSERT (r.begin (def3, &elem));
                TEST_ASSERT (15 == r.dataBlob_getSize());
                TEST_ASSERT (eDataBlobElemType::structType == elem.getType());
                TEST_ASSERT (3 == elem.structType_getNumMembers());
                TEST_ASSERT (false == elem.next());
                            
            gos::datablob::print_info ("def4", def4);
            TEST_ASSERT (r.begin (def4, &elem));
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

            gos::datablob::print_info ("def5", def5);
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