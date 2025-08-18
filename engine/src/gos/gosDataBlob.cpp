#include "gosDataBlob.h"
#include "gos.h"

using namespace gos;
using namespace gos::datablob;

//******************************** 
void sElemHeader::decodeFromBuffer (const u8 *buffer)
{
    BufferR reader;
    reader.setup (buffer, 256, eEndianess::big);

    this->elemType = static_cast<eDataBlobElemType> (reader.readU8());
    this->nameLen = reader.readU8 ();
    this->next = reader.readU16 ();
    this->absOffset = reader.readU16 ();
    this->paddedSize = reader.readU16 ();

    this->elemName = NULL;
    if (this->nameLen)
    {
        this->elemName = reinterpret_cast<const char*>(reader.getPointer (reader.tell()));
        reader.advanceCursor (nameLen);
    }

    this->sizeof_thisHeader = reader.tell();
}

//******************************** 
bool gos::datablob::blobDef_isValidMagic (const void *dataBlodDef)
{
    assert (NULL != dataBlodDef);
    const u32 magic = gos::utils::bufferReadU32 (reinterpret_cast<const u8*>(dataBlodDef));
    if (!gos::magic::signatureMatch (magic, GOS_MAGIC__DATA_BLOB_DEF))
        return false;
    if (!gos::magic::versionMatch (magic, GOS_MAGIC__DATA_BLOB_DEF))
        return false;
    return true;
}

//******************************** 
u16 gos::datablob::blobDef_getTotalSize (const void *dataBlodDef)
{
    assert (blobDef_isValidMagic(dataBlodDef));
    const u8 *p = reinterpret_cast<const u8*>(dataBlodDef);
    return gos::utils::bufferReadU16 (&p[4]);
}

//******************************** 
u16 gos::datablob::blobDef_getSizeOfDataBlob (const void *dataBlodDef)
{
    assert (blobDef_isValidMagic(dataBlodDef));
    const u8 *p = reinterpret_cast<const u8*>(dataBlodDef);
    return gos::utils::bufferReadU16 (&p[6]);
}

//******************************** 
static void gos_datablob_print_info_header (const gos::datablob::DefReader::Elem &elem, const char *namePrefix, u16 indent)
{
    char var_name[256];

    memset (var_name, 0, sizeof(var_name));
    if (indent)
        memset (var_name, ' ', indent);

    if (NULL == namePrefix)
        sprintf_s (&var_name[indent], sizeof(var_name) - indent, "%s", elem.getName());
    else
        sprintf_s (&var_name[indent], sizeof(var_name) - indent, "%s.%s", namePrefix, elem.getName());

    switch (elem.getType())
    {
    default:
        gos::logger::log ("ERROR!!\n");
        DBGBREAK;
        break;

    case eDataBlobElemType::simpleType:
        gos::logger::log ("%-6d %-4d %-8s %s\n", 
                            elem.getOffset(), elem.getPaddedSize(),
                            gos::utils::enumToString(elem.simpleType_getDataFmt()), var_name);
        break;

    case eDataBlobElemType::structType:
        gos::logger::log ("%-6d %-4d %-8s %s (num-members=%d)\n", 
                            elem.getOffset(), elem.getPaddedSize(),
                            "struct", var_name,
                            elem.structType_getNumMembers());
        {
            gos::datablob::DefReader::Elem childElem;
            elem.structType_getFirstMember (&childElem);
            do
            {
                gos_datablob_print_info_header (childElem, var_name, indent+2);
            } while (childElem.next());
        }
        break;

    case eDataBlobElemType::arrayType:
        {
            const u8 dimension = elem.arrayType_getNumDimension();
            for (u8 i=0; i<dimension; i++)
            {
                char s[32];
                sprintf_s (s, sizeof(s), "[%d]", elem.arrayType_getNumElem(i));
                strcat_s (var_name, sizeof(var_name), s);
            }

            gos::logger::log ("%-6d %-4d %-8s %s (size-of-oneElem=%d)\n", 
                                elem.getOffset(), elem.getPaddedSize(),
                                "array", var_name,
                                elem.arrayType_getSizeOfOneElem());
            {
                gos::datablob::DefReader::Elem childElem;
                elem.arrayType_getFirstMember (&childElem);
                do
                {
                    gos_datablob_print_info_header (childElem, var_name, indent+2);
                } while (childElem.next());
            }
        }
        break;
    }
}

//******************************** 
void gos::datablob::print_info (const char *name, const void *dataBlobDef)
{
    gos::datablob::DefReader r;
    gos::datablob::DefReader::Elem elem;

    if (!r.begin (dataBlobDef, &elem))
    {
        DBGBREAK;
        return;
    }
    gos::logger::log ("================================\n");
    gos::logger::log ("%s, dataBlofDef_size=%d, dataBlob_size=%d\n", name, gos::datablob::blobDef_getTotalSize(dataBlobDef), r.dataBlob_getSize());
    gos::logger::incIndent();
    gos::logger::log ("OFFSET SIZE TYPE     VAR-NAME\n");
    do
    {
        gos_datablob_print_info_header (elem, NULL, 0);
    } while (elem.next());        

    gos::logger::log("\n");
    gos::logger::decIndent();
}

