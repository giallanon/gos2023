#include "gosDataBlob.h"
#include "gos.h"

using namespace gos;
using namespace gos::datablob;

//******************************** 
DefBuilder::DefBuilder()
{
    buffer.setup (gos::getSysHeapAllocator(), 256, eEndianess::big);
    bIsValid = false;
}

//******************************** 
DefBuilder& DefBuilder::begin()
{
    bIsValid = false;
    sizeof_dataBlob = 0;
    buffer.reset();
    stack.reset();

    buffer.writeU32 (GOS_MAGIC__DATA_BLOB_DEF);
    buffer.writeU16 (0);
    buffer.writeU16 (0);

    return *this;
}

//******************************** 
bool DefBuilder::end()
{
    //scrive la dimensione totale di questo DataBlobDef
    const u16 total_size_of_dataBlobDef = static_cast<u16>(buffer.tell());
    buffer.writeU16At (4, total_size_of_dataBlobDef);

    //scrive la dimensione totale del DataBlob descritto da questo DataBlobDef
    buffer.writeU16At (6, sizeof_dataBlob);

    bIsValid = true;
    return true;
}

//******************************** 
bool DefBuilder::memcpyDataBlobDef (void *dst, u32 sizeof_dst)
{
    const u32 size = getDataBlobDefSize();
    if (sizeof_dst >= size)
    {
        buffer.readAtAndCopyHere (0, dst, size);
        return true;
    }

    return false;
}

//******************************** 
u16 DefBuilder::priv_elem_begin (eDataBlobElemType elemtype, const char *name)
{
    const u16 pos = static_cast<u16>(buffer.tell());
    stack.push (pos);

    buffer.writeU8 (static_cast<u8>(elemtype));

    u8 nameLen = 0;
    if (NULL != name)
    {
        if (0x00 != name[0])
            nameLen = static_cast<u8>(1 + strlen(name) );
    }
    buffer.writeU8 (nameLen);

    buffer.writeU16 (0xFFFF);           //next
    buffer.writeU16 (sizeof_dataBlob);  //absOffset
    buffer.writeU16 (0x0000);           //size

    if (nameLen)
        buffer.write (name, nameLen);   //name

    return buffer.tell();
}

//******************************** 
u16 DefBuilder::priv_elem_end ()
{
    u16 pos_elemStarted;
    stack.pop (&pos_elemStarted);

    const u32 pos_now = buffer.tell();

    //fillo il campo 'next' che ho creato durante elem_begin
    buffer.writeU16At (pos_elemStarted+2, pos_now);

    return pos_elemStarted;
}


//******************************** 
DefBuilder& DefBuilder::add_simpleType (const char *var_name, eDataFormat fmt, u32 paddedSize)
{
    priv_elem_begin (eDataBlobElemType::simpleType, var_name);

    //scrivo il blocco data specifico di questo elemento
    buffer.writeU8 (static_cast<u8>(fmt));

    const u32 pos_elemStarted = priv_elem_end();


    //scrivo la dimensione di questo data-type
    if (u32MAX == paddedSize)
        paddedSize = gos::dataformat::getSize(fmt);
    buffer.writeU16At (pos_elemStarted+6, static_cast<u16>(paddedSize));

    sizeof_dataBlob += paddedSize;
    return *this;
}

//******************************** 
DefBuilder& DefBuilder::struct_begin (const char *var_name)
{
    priv_elem_begin (eDataBlobElemType::structType, var_name);
    stack.push (sizeof_dataBlob);

    //num members
    buffer.writeU8 (0);

    return *this;
}

//******************************** 
DefBuilder& DefBuilder::struct_add_simpleType (const char *var_name, eDataFormat fmt, u32 paddedSize)
{
    add_simpleType (var_name, fmt, paddedSize);
    return *this;
}

//******************************** 
DefBuilder& DefBuilder::struct_end()
{
    u16 sizeof_dataBlob_beforeThisStruct;
    stack.pop (&sizeof_dataBlob_beforeThisStruct);
    const u32 pos_elemStarted = priv_elem_end();

    //scrivo la dimensione di questo data-type
    const u16 sizeof_thisStruct = sizeof_dataBlob - sizeof_dataBlob_beforeThisStruct;
    buffer.writeU16At (pos_elemStarted+6, sizeof_thisStruct);

    //devo calcolare il num di membri della struct
    sElemHeader header;
    header.decodeFromBuffer (buffer.getPointer(pos_elemStarted));

    const u32 pos_dataBlock = pos_elemStarted + header.sizeof_thisHeader;
    u32 pos = pos_dataBlock+1;
    u8 numMembers = 0;
    while (pos < buffer.tell())
    {
        header.decodeFromBuffer (buffer.getPointer(pos));
        pos = header.next;
        numMembers++;
    }
    buffer.writeU8At (pos_dataBlock, numMembers);

    return *this;
}

