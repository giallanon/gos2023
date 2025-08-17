#include "gosDataBlob.h"
#include "gos.h"

using namespace gos;
using namespace gos::datablob;

//*****************************
DefReader::DefReader()
{
}

//*****************************
bool DefReader::begin (const void *dataBlobDef)
{
    reader.setup (dataBlobDef, 8, eEndianess::big);

    const u32 magic = reader.readU32();
    if (!gos::magic::signatureMatch (magic, GOS_MAGIC__DATA_BLOB_DEF))
        return false;
    if (!gos::magic::versionMatch (magic, GOS_MAGIC__DATA_BLOB_DEF))
        return false;

    const u32 sizeof_blobDef = reader.readU16();
    reader.setup (dataBlobDef, sizeof_blobDef, eEndianess::big);
    
    pos_curElem = 8;
    curElemHeader.decodeFromBuffer (reader.getPointer(pos_curElem));
    return true;
}

//*****************************
bool DefReader::nextElem()
{
    const u16 pos_nextElem = reader.readU16At (pos_curElem+2);

    if (pos_nextElem >= reader.readU16At(4))
        return false;

    pos_curElem = pos_nextElem;
    curElemHeader.decodeFromBuffer (reader.getPointer(pos_curElem));
    return true;
}

//*****************************
eDataFormat DefReader::simpleType_getDataFmt() const
{
    assert (elem_getType() == eDataBlobElemType::simpleType);
    const u32 pos_startOfDataBlock = pos_curElem + curElemHeader.sizeof_thisHeader;

    return static_cast<eDataFormat> (reader.readU8At (pos_startOfDataBlock));
}

//*****************************
u8 DefReader::structType_getNumMembers() const
{
    assert (elem_getType() == eDataBlobElemType::structType);
    const u32 pos_startOfDataBlock = pos_curElem + curElemHeader.sizeof_thisHeader;

    return reader.readU8At (pos_startOfDataBlock);
}

//*****************************
const char* DefReader::structType_getMemberName(u8 index) const
{
    assert (elem_getType() == eDataBlobElemType::structType);
    assert (index < structType_getNumMembers());

    sElemHeader header;
    u32 pos = pos_curElem + curElemHeader.sizeof_thisHeader + 1;
    header.decodeFromBuffer (reader.getPointer(pos));
    
    for (u8 i=0;i<index;i++)
    {
        pos = header.next;
        header.decodeFromBuffer (reader.getPointer(pos));
    }

    return header.elemName;
}