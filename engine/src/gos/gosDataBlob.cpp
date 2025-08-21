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
    this->userDefined = reader.readU32 ();

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
u16 gos::datablob::blobDef_getSize (const void *dataBlodDef)
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
u8* gos::datablob::createNew (gos::Allocator *allocator, const void *dataBlobDef)
{
    const u16 size = blobDef_getSizeOfDataBlob(dataBlobDef);
    u8 *ret = GOSALLOCT(u8*, allocator, size);
    memset (ret, 0, size);
    return ret;
}

//******************************** 
void gos::datablob::destroy (gos::Allocator *allocator, void *dataBlob)
{
    if (NULL != dataBlob)
        GOSFREE(allocator, dataBlob);
}


//******************************** 
void gos_datablob_blobDef_prinfInfo_ric (gos::UTF8String &out, gos::datablob::DefElem &elem, u32 indent, trapFn_printOtherInfoOnThisRow trapFn)
{
    static constexpr u8 PRINT_COL1 = 45;
    static constexpr u8 PRINT_COL2 = 72;
    //static constexpr u8 PRINT_COL3 = 100;

    char sIndent[128];
    memset (sIndent, 0, sizeof(sIndent));
    if (indent)
        memset(sIndent, ' ', indent*4);

    if (eDataBlobElemType::structType == elem.getType())
    {
        out << "\n" << sIndent << "struct\n"
            << sIndent << "{\n";

        DefElem figlio;
        if (elem.getFirstChild(&figlio))
            gos_datablob_blobDef_prinfInfo_ric (out, figlio, indent+1, trapFn);

        out << sIndent << "} " << elem.getName() << ";";
        
        if (trapFn)
        {
            out.fillRowUntilColumn (PRINT_COL2);
            trapFn (out, elem);
        }
        out << "\n\n";
    }
    else if (eDataBlobElemType::arrayType == elem.getType())
    {
        char arrName[64];
        sprintf_s (arrName, sizeof(arrName), "%s", elem.getName());
        for (u8 i=0; i<elem.arrayType_getNumDimension(); i++)
        {
            char s[32];
            sprintf_s (s, sizeof(s), "[%d]", elem.arrayType_getNumElem(i));
            strcat_s (arrName, sizeof(arrName), s);
        }

        DefElem figlio;
        if (elem.getFirstChild(&figlio))
        {
            DefElem figlio;
            elem.getFirstChild(&figlio);

            if (elem.arrayType_isSimple())
            {
                out << sIndent << STRFMT("%-10s", gos::utils::enumToString(figlio.getDataFmt())) << arrName << ";";
            }
            else
            {
                //array di struct
                out << "\n" << sIndent << "struct\n"
                    << sIndent << "{\n";

                gos_datablob_blobDef_prinfInfo_ric (out, figlio, indent+1, trapFn);

                out << sIndent << "} " << arrName << ";";
            }

            out.fillRowUntilColumn (PRINT_COL1-1);
            out << "|" << STRFMT("% 6d", elem.getOffset()) << "|"
                << STRFMT("% 8d", elem.getPaddedSize()) << "|"
                << "stride=" << elem.arrayType_getStride();

            if (trapFn)
            {
                out.fillRowUntilColumn (PRINT_COL2);
                trapFn (out, elem);
            }
            out << "\n\n";
        }
    }
    else 
    {
        //variabile semplice
        out << sIndent << STRFMT("%-10s", gos::utils::enumToString(elem.getDataFmt())) << elem.getName() << ";";
        out.fillRowUntilColumn (PRINT_COL1-1);
        out << "|" << STRFMT("% 6d", elem.getOffset()) << "|"
            << STRFMT("% 8d", elem.getPaddedSize()) << "|";

        if (trapFn)
        {
            out.fillRowUntilColumn (PRINT_COL2);
            trapFn (out, elem);
        }
        out << "\n";
    }

    if (elem.next())
        gos_datablob_blobDef_prinfInfo_ric (out, elem, indent, trapFn);
}

void gos::datablob::blobDef_prinfInfo (gos::UTF8String &out, const void *dataBlobDef, trapFn_printOtherInfoOnThisRow trapFn)
{
    gos::datablob::DefReader r;
    if (!r.setup (dataBlobDef))
    {
        DBGBREAK;
        return;
    }

    gos::datablob::DefElem elem;    
    r.beginEnumerate (&elem);
    out << "sizeof_dataBlobDef=" << datablob::blobDef_getSize(dataBlobDef) << ", sizeof_dataBlob=" << r.dataBlob_getSize()
        << "\n"
        << "NAME                                        |OFFSET|PAD-SIZE|OTHER\n"
        << "------------------------------------------------------------------\n";

    do
    {
        gos_datablob_blobDef_prinfInfo_ric (out, elem, 0, trapFn);
    } while (elem.next());
    out << "\n";
}
