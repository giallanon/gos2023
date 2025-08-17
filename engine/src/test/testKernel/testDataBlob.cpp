#include "TTest.h"
#include "gosDataBlob.h"

namespace test_datablob
{

void print_info_struct (const gos::datablob::DefReader &r)
{
    const u8 numMembers = r.structType_getNumMembers();
    gos::logger::log ("num-members: %d\n", numMembers);
    for (u8 i=0; i<numMembers; i++)
        gos::logger::log ("%s\n", r.structType_getMemberName(i));
}

void print_info_header (const gos::datablob::DefReader &r)
{
    gos::logger::log ("%-8s, offset=% 4d, size=% 4d, type= ", 
        r.elem_getName(),
        r.elem_getOffset(),
        r.elem_getPaddedSize()
        );

    switch (r.elem_getType())
    {
    default:
        gos::logger::log ("ERROR!!\n");
        DBGBREAK;
        break;

    case eDataBlobElemType::simpleType:
        gos::logger::log ("simpleType, fmt=%s\n", gos::utils::enumToString(r.simpleType_getDataFmt()));
        break;

    case eDataBlobElemType::structType:
        gos::logger::log ("structType\n");
        gos::logger::incIndent();
        print_info_struct(r);
        gos::logger::decIndent();
        break;
    }
}

void print_info (const char *name, const void *dataBlobDef)
{
    gos::datablob::DefReader r;

    r.begin (dataBlobDef);
    gos::logger::log ("================================\n");
    gos::logger::log ("%s, blobSize=%d\n", name, r.dataBlob_getSize());
    gos::logger::incIndent();
    do
    {
        print_info_header (r);
    } while (r.nextElem());        

    gos::logger::log("\n");
    gos::logger::decIndent();
}

namespace test1
{
    int run()
    {
        //def builder
        u8  def1[64];
        u8  def2[64];
        u8  def3[128];
        u8  def4[128];
        {
            gos::datablob::DefBuilder def;

            TEST_ASSERT (false == def.isValid());
            def.begin()
                .add_simpleType ("s1", eDataFormat::_1f32)
                .end();
            TEST_ASSERT (true == def.isValid());
            TEST_ASSERT (def.getDataBlobDefSize() == 8 + 12);
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
            TEST_ASSERT (def.getDataBlobDefSize() == 56);
            TEST_ASSERT (def.memcpyDataBlobDef (def2, sizeof(def2)));

            def.begin();
            TEST_ASSERT (false == def.isValid());
            def
                .struct_begin ("struct1")
                    .struct_add_simpleType ("m1", eDataFormat::_2u8)
                    .struct_add_simpleType ("m2", eDataFormat::_1i8)
                    .struct_add_simpleType ("m3", eDataFormat::_3f32)
                .struct_end()
            .end();
            TEST_ASSERT (true == def.isValid());
            TEST_ASSERT (def.memcpyDataBlobDef (def3, sizeof(def3)));

            def.begin();
            TEST_ASSERT (false == def.isValid());
            def
                .add_simpleType ("s1", eDataFormat::_2f32)
                .struct_begin ("struct1")
                    .struct_add_simpleType ("m1", eDataFormat::_2u8)
                    .struct_add_simpleType ("m2", eDataFormat::_1i8)
                    .struct_add_simpleType ("m3", eDataFormat::_3f32)
                    .struct_add_simpleType ("m4", eDataFormat::_1i8)
                .struct_end()
                .add_simpleType ("s2", eDataFormat::_1i32)
            .end();
            TEST_ASSERT (true == def.isValid());
            TEST_ASSERT (def.memcpyDataBlobDef (def4, sizeof(def4)));
        }

        //DefReader
        {
            u32 nElem;
            gos::datablob::DefReader r;

            print_info ("def1", def1);
            TEST_ASSERT (r.begin (def1));
            nElem = 0;
            do
            {
                TEST_ASSERT (eDataBlobElemType::simpleType == r.elem_getType());
                TEST_ASSERT (0 == r.elem_getOffset());
                TEST_ASSERT (4 == r.elem_getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s1", r.elem_getName()));
                TEST_ASSERT (eDataFormat::_1f32 == r.simpleType_getDataFmt());
                nElem++;
            } while (r.nextElem());
            TEST_ASSERT(1 == nElem);


            print_info ("def2", def2);
            TEST_ASSERT (r.begin (def2));
            nElem = 0;
                TEST_ASSERT (eDataBlobElemType::simpleType == r.elem_getType());
                TEST_ASSERT (0 == r.elem_getOffset());
                TEST_ASSERT (4 == r.elem_getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s2", r.elem_getName()));
                TEST_ASSERT (eDataFormat::_1u32 == r.simpleType_getDataFmt());
                TEST_ASSERT (r.nextElem());
                nElem++;
            
                TEST_ASSERT (eDataBlobElemType::simpleType == r.elem_getType());
                TEST_ASSERT (4 == r.elem_getOffset());
                TEST_ASSERT (8 == r.elem_getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s3", r.elem_getName()));
                TEST_ASSERT (eDataFormat::_2u32 == r.simpleType_getDataFmt());
                TEST_ASSERT (r.nextElem());
                nElem++;

                TEST_ASSERT (eDataBlobElemType::simpleType == r.elem_getType());
                TEST_ASSERT (12 == r.elem_getOffset());
                TEST_ASSERT (12 == r.elem_getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s4", r.elem_getName()));
                TEST_ASSERT (eDataFormat::_3u32 == r.simpleType_getDataFmt());
                TEST_ASSERT (r.nextElem());
                nElem++;

                TEST_ASSERT (eDataBlobElemType::simpleType == r.elem_getType());
                TEST_ASSERT (24 == r.elem_getOffset());
                TEST_ASSERT (16 == r.elem_getPaddedSize());
                TEST_ASSERT (0 == strcmp ("s5", r.elem_getName()));
                TEST_ASSERT (eDataFormat::_4u32 == r.simpleType_getDataFmt());
                TEST_ASSERT (false == r.nextElem());
                nElem++;                                            
            TEST_ASSERT(4 == nElem);


            print_info ("def3", def3);
            TEST_ASSERT (r.begin (def3));
                TEST_ASSERT (15 == r.dataBlob_getSize());
                TEST_ASSERT (eDataBlobElemType::structType == r.elem_getType());
                TEST_ASSERT (3 == r.structType_getNumMembers());
                TEST_ASSERT (false == r.nextElem());
                            
            print_info ("def4", def4);
            TEST_ASSERT (r.begin (def4));
                TEST_ASSERT (28 == r.dataBlob_getSize());
                TEST_ASSERT (eDataBlobElemType::simpleType == r.elem_getType());
                TEST_ASSERT (r.nextElem());
                
                TEST_ASSERT (eDataBlobElemType::structType == r.elem_getType());
                TEST_ASSERT (4 == r.structType_getNumMembers());
                TEST_ASSERT (r.nextElem());

                TEST_ASSERT (eDataBlobElemType::simpleType == r.elem_getType());
                TEST_ASSERT (false == r.nextElem());

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