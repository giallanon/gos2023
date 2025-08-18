#include "gosDataBlob.h"
#include "gos.h"

using namespace gos;
using namespace gos::datablob;

//*****************************
void DefReader::Elem::priv_setup (const BufferR *readerIN, u16 startingPos, u16 endingPosIN)
{
    reader = readerIN;
    header.decodeFromBuffer (reader->getPointer(startingPos));
    pos_curElem = startingPos;
    endingPos = endingPosIN;
}

//*****************************
bool DefReader::Elem::next()
{
    const u16 pos_nextElem = header.next;
    if (pos_nextElem >= endingPos)
        return false;

    pos_curElem = pos_nextElem;
    header.decodeFromBuffer (reader->getPointer(pos_curElem));
    return true;
}

//*****************************
eDataFormat DefReader::Elem::simpleType_getDataFmt() const
{
    assert (getType() == eDataBlobElemType::simpleType);
    const u32 pos_startOfDataBlock = pos_curElem + header.sizeof_thisHeader;

    return static_cast<eDataFormat> (reader->readU8At (pos_startOfDataBlock));
}

//*****************************
u8 DefReader::Elem::structType_getNumMembers() const
{
    assert (getType() == eDataBlobElemType::structType);
    const u32 pos_startOfDataBlock = pos_curElem + header.sizeof_thisHeader;

    return reader->readU8At (pos_startOfDataBlock + 4);
}

//*****************************
const char* DefReader::Elem::structType_getMemberName(u8 index) const
{
    assert (getType() == eDataBlobElemType::structType);
    assert (index < structType_getNumMembers());

    u16 pos = reader->readU16At (pos_curElem + header.sizeof_thisHeader + 2);
    sElemHeader tempHeader;
    tempHeader.decodeFromBuffer (reader->getPointer(pos));
    
    for (u8 i=0;i<index;i++)
    {
        pos = tempHeader.next;
        tempHeader.decodeFromBuffer (reader->getPointer(pos));
    }

    return tempHeader.elemName;
}

//*****************************
bool DefReader::Elem::structType_getFirstMember (Elem *out) const
{
    assert (NULL != out);
    assert (getType() == eDataBlobElemType::structType);
    const u16 posFistChild = reader->readU16At (pos_curElem + header.sizeof_thisHeader);
    const u16 endPosOfLastChild = reader->readU16At (pos_curElem + header.sizeof_thisHeader + 2);
    out->priv_setup (reader, posFistChild, endPosOfLastChild);
    return true;
}

//*****************************
u8 DefReader::Elem::arrayType_getNumDimension() const
{
    assert (getType() == eDataBlobElemType::arrayType);
    const u32 pos_startOfDataBlock = pos_curElem + header.sizeof_thisHeader;

    return reader->readU8At (pos_startOfDataBlock + 4);
}

//*****************************
u16 DefReader::Elem::arrayType_getNumElem (u8 index) const
{
    assert (getType() == eDataBlobElemType::arrayType);
    assert (index < arrayType_getNumDimension());
    const u32 pos_startOfDataBlock = pos_curElem + header.sizeof_thisHeader;

    return reader->readU16At (pos_startOfDataBlock + 6 + index*2);
}

//*****************************
u8 DefReader::Elem::arrayType_getSizeOfOneElem() const
{
    assert (getType() == eDataBlobElemType::arrayType);
    const u32 pos_startOfDataBlock = pos_curElem + header.sizeof_thisHeader;

    return reader->readU8At (pos_startOfDataBlock + 5);
}

//*****************************
bool DefReader::Elem::arrayType_getFirstMember (Elem *out) const
{
    assert (NULL != out);
    assert (getType() == eDataBlobElemType::arrayType);
    const u16 posFistChild = reader->readU16At (pos_curElem + header.sizeof_thisHeader);
    const u16 endPosOfLastChild = reader->readU16At (pos_curElem + header.sizeof_thisHeader + 2);
    out->priv_setup (reader, posFistChild, endPosOfLastChild);
    return true;
}


//*****************************
bool DefReader::begin (const void *dataBlobDef, Elem *out)
{
    assert (NULL != out);

    //verifico che il blob sia in effetti una DataBlobDef
    if (!datablob::blobDef_isValidMagic(dataBlobDef))
        return false;

    //preparo il reader
    const u16 sizeof_blobDef = datablob::blobDef_getTotalSize(dataBlobDef);
    reader.setup (dataBlobDef, sizeof_blobDef, eEndianess::big);
    
    //e mi porto sul primo elemento
    out->priv_setup (&reader, 8, sizeof_blobDef);
    return true;
}

