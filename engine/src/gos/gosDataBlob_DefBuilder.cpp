#include "gosDataBlob.h"
#include "gos.h"

using namespace gos;
using namespace gos::datablob;

//******************************** 
DefBuilder::DefBuilder()
{
    buffer.setup (gos::getSysHeapAllocator(), 256, eEndianess::big);
    flag.zero();
    flag.set (FLAG__ERROR);
}

//******************************** 
DefBuilder& DefBuilder::begin()
{
    flag.zero();
    flag.set (FLAG__BEGIN);

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
    if (flag.isBitSet(FLAG__ERROR))
        return false;
    if (!flag.isBitSet(FLAG__BEGIN))
        return false;

    //scrive la dimensione totale di questo DataBlobDef
    const u16 total_size_of_dataBlobDef = static_cast<u16>(buffer.tell());
    buffer.writeU16At (4, total_size_of_dataBlobDef);

    //scrive la dimensione totale del DataBlob descritto da questo DataBlobDef
    buffer.writeU16At (6, sizeof_dataBlob);

    flag.zero();
    return true;
}

//******************************** 
bool DefBuilder::isValid() const
{
    if (flag.isBitSet(FLAG__BEGIN))
        return false;
    if (flag.isBitSet(FLAG__ERROR))
        return false;
    return true;
}

//******************************** 
bool DefBuilder::memcpyDataBlobDef (void *dst, u32 sizeof_dst) const
{
    if (!isValid())
        return false;

    const u32 size = getDataBlobDefSize();
    if (sizeof_dst >= size)
    {
        buffer.readAtAndCopyHere (0, dst, size);
        return true;
    }

    return false;
}

//******************************** 
u8* DefBuilder::allocDataBlobDef (gos::Allocator *allocator) const
{
    if (!isValid())
        return NULL;

    const u32 size = getDataBlobDefSize();
    u8 *ret = GOSALLOCT(u8*, allocator, size);
    buffer.readAtAndCopyHere (0, ret, size);
    return ret;
}

//******************************** 
void DefBuilder::priv_add_pad()
{
    u32 pad = buffer.tell() % 8;
    if (0 == pad)
        return;
    pad = 8 - pad;
    while (pad--)
        buffer.writeU8(0);
}

//******************************** 
u16 DefBuilder::priv_elem_begin (eDataBlobElemType elemtype, const char *name, u32 userDefinedData)
{
    //voglio che parta ad un indirizzo che e' un multiplo di 8, in modo che
    //il "name" sia allineato correttamente
    assert (buffer.tell() % 8 == 0);

    const u16 pos = static_cast<u16>(buffer.tell());
    stack.push (pos);

    //0     elem type
    buffer.writeU8 (static_cast<u8>(elemtype));

    //1     nameLen
    u8 nameLen = 0;
    if (NULL != name)
    {
        if (0x00 != name[0])
            nameLen = static_cast<u8>(1 + strlen(name) );
    }
    buffer.writeU8 (nameLen);

    //2     next
    buffer.writeU16 (0xFFFF);
    
    //4     absOffset
    buffer.writeU16 (sizeof_dataBlob);

    //6     size
    buffer.writeU16 (0x0000);

    //8     userDefine
    buffer.writeU32 (userDefinedData);

    //12    name
    if (nameLen)
        buffer.write (name, nameLen);   //name

    return buffer.tell();
}

//******************************** 
u16 DefBuilder::priv_elem_end ()
{
    u16 pos_elemStarted;
    stack.pop (&pos_elemStarted);

    //voglio che il prossimo elemento parta ad un indirizzo che sia un multiplo di 8
    //per questioni di allineamento del campo "name"
    priv_add_pad();

    //fillo il campo 'next' che ho creato durante elem_begin
    buffer.writeU16At (pos_elemStarted+2, buffer.tell());

    return pos_elemStarted;
}

//******************************** 
DefBuilder& DefBuilder::add_simpleTypeAtOffset (u16 offset, const char *var_name, eDataFormat fmt, u32 userDefinedData, u32 paddedSize)
{
    if (!flag.isBitSet(FLAG__BEGIN))
        flag.set (FLAG__ERROR);
    if (offset < sizeof_dataBlob)
        flag.set(FLAG__ERROR);
    if (flag.isBitSet(FLAG__ERROR))
    {
        DBGBREAK;
        return *this;
    }
    sizeof_dataBlob = offset;

    priv_elem_begin (eDataBlobElemType::simpleType, var_name, userDefinedData);

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
DefBuilder& DefBuilder::struct_beginAtOffset (u16 offset, const char *var_name, u32 userDefinedData)
{
    if (!flag.isBitSet(FLAG__BEGIN))
        flag.set (FLAG__ERROR);
    if (offset < sizeof_dataBlob)
        flag.set(FLAG__ERROR);
    if (flag.isBitSet(FLAG__ERROR))
    {
        DBGBREAK;
        return *this;
    }

    sizeof_dataBlob = offset;

    priv_elem_begin (eDataBlobElemType::structType, var_name, userDefinedData);
    stack.push (sizeof_dataBlob);

    //pos of 1st child
    const u16 pos_posOfFirstChild = buffer.tell();
    buffer.writeU16 (0);

    //end pos of last child
    buffer.writeU16 (0);

    //num members
    buffer.writeU16 (0);

    //voglio che il prossimo elemento parta ad un indirizzo che sia un multiplo di 8
    //per questioni di allineamento del campo "name"
    priv_add_pad();

    buffer.writeU16At (pos_posOfFirstChild, buffer.tell());
    return *this;
}

//******************************** 
DefBuilder& DefBuilder::struct_end()
{
    if (!flag.isBitSet(FLAG__BEGIN))
        flag.set (FLAG__ERROR);
    if (flag.isBitSet(FLAG__ERROR))
    {
        DBGBREAK;
        return *this;
    }


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
    const u16 pos_firstChild = buffer.readU16At (pos_dataBlock);
    
    u32 pos = pos_firstChild;
    u8 numMembers = 0;
    while (pos < buffer.tell())
    {
        header.decodeFromBuffer (buffer.getPointer(pos));
        pos = header.next;
        numMembers++;
    }
    buffer.writeU8At (pos_dataBlock + 4, numMembers);

    //scrivo la end pos dell'ultimo figlio
    buffer.writeU16At (pos_dataBlock + 2, static_cast<u16>(buffer.tell()));
    return *this;
}

//******************************** 
DefBuilder& DefBuilder::array_begin1DAtOffset (u16 offset, const char *var_name, u16 numElem1, u32 userDefinedData)
{
    const u16 pos_posOfFirstChild = priv_array_begin_start (offset, var_name, 1, userDefinedData);
    if (u16MAX != pos_posOfFirstChild)
    {
        buffer.writeU16 (numElem1);
        priv_array_begin_end (pos_posOfFirstChild);
    }
    return *this;
}

//******************************** 
DefBuilder& DefBuilder::array_begin2DAtOffset (u16 offset, const char *var_name, u16 numElem1, u16 numElem2, u32 userDefinedData)
{
    const u16 pos_posOfFirstChild = priv_array_begin_start (offset, var_name, 2, userDefinedData);
    if (u16MAX != pos_posOfFirstChild)
    {
        buffer.writeU16 (numElem1);
        buffer.writeU16 (numElem2);
        priv_array_begin_end (pos_posOfFirstChild);
    }
    return *this;
}

//******************************** 
DefBuilder& DefBuilder::array_begin3DAtOffset (u16 offset, const char *var_name, u16 numElem1, u16 numElem2, u16 numElem3, u32 userDefinedData)
{
    const u16 pos_posOfFirstChild = priv_array_begin_start (offset, var_name, 3, userDefinedData);
    if (u16MAX != pos_posOfFirstChild)
    {
        buffer.writeU16 (numElem1);
        buffer.writeU16 (numElem2);
        buffer.writeU16 (numElem3);
        priv_array_begin_end (pos_posOfFirstChild);
    }
    return *this;
}

//******************************** 
u16 DefBuilder::priv_array_begin_start (u16 offset, const char *var_name, u8 numDimension, u32 userDefinedData)
{
    if (!flag.isBitSet(FLAG__BEGIN))
        flag.set (FLAG__ERROR);
    if (offset < sizeof_dataBlob)
        flag.set(FLAG__ERROR);
    if (flag.isBitSet(FLAG__ERROR))
    {
        DBGBREAK;
        return u16MAX;
    }

    sizeof_dataBlob = offset;

    priv_elem_begin (eDataBlobElemType::arrayType, var_name, userDefinedData);
    stack.push (sizeof_dataBlob);

    //pos of 1st child
    const u16 pos_posOfFirstChild = buffer.tell();
    buffer.writeU16 (0);

    //end pos of last child
    buffer.writeU16 (0);

    //dimension
    buffer.writeU8 (numDimension);

    //size of one elem
    buffer.writeU8 (0);

    return pos_posOfFirstChild;
}

void DefBuilder::priv_array_begin_end (u16 pos_posOfFirstChild)
{
    if (!flag.isBitSet(FLAG__BEGIN))
        flag.set (FLAG__ERROR);
    if (flag.isBitSet(FLAG__ERROR))
    {
        DBGBREAK;
        return;
    }


    //voglio che il prossimo elemento parta ad un indirizzo che sia un multiplo di 8
    //per questioni di allineamento del campo "name"
    priv_add_pad();

    buffer.writeU16At (pos_posOfFirstChild, buffer.tell());
}

//******************************** 
DefBuilder& DefBuilder::array_end ()
{
    if (!flag.isBitSet(FLAG__BEGIN))
        flag.set (FLAG__ERROR);
    if (flag.isBitSet(FLAG__ERROR))
    {
        DBGBREAK;
        return *this;
    }


    u16 sizeof_dataBlob_beforeThisStruct;
    stack.pop (&sizeof_dataBlob_beforeThisStruct);
    const u32 pos_elemStarted = priv_elem_end();

    //calcolo la dimensione di un singolo elemento dell'array
    const u8 sizeof_oneElem = static_cast<u8>(sizeof_dataBlob - sizeof_dataBlob_beforeThisStruct);

    //mi posiziono sul data block
    sElemHeader header;
    header.decodeFromBuffer (buffer.getPointer(pos_elemStarted));
    const u32 pos_dataBlock = pos_elemStarted + header.sizeof_thisHeader;

    //scrivo lo stride
    buffer.writeU8At (pos_dataBlock+5, sizeof_oneElem);

    //calcolo la dimensione totale dell'array
    const u8 dimension = buffer.readU8At (pos_dataBlock+4);
    u32 total_sizeof_array = sizeof_oneElem * buffer.readU16At (pos_dataBlock+6);
    for (u8 i=1; i<dimension; i++)
    {
        const u16 n = buffer.readU16At (pos_dataBlock+6 + 2*i);
        total_sizeof_array *= n;
    }
    buffer.writeU16At (pos_elemStarted+6, static_cast<u16>(total_sizeof_array));

    //aggiusto la dimensione del blob
    sizeof_dataBlob -= sizeof_oneElem;
    sizeof_dataBlob += total_sizeof_array;

    //scrivo la end pos dell'ultimo figlio
    buffer.writeU16At (pos_dataBlock + 2, static_cast<u16>(buffer.tell()));

    //conto il numero di figli. Se ha solo un figlio, allora deve essere di tipo
    //eDataBlobElemType::simpleType e significa che l'array e' un array semplice.
    //Se ha piu' di un figlio, allora e' un array di struct
    u16 pos = buffer.readU16At (pos_dataBlock);
    u8 numMembers = 0;
    while (pos < buffer.tell())
    {
        header.decodeFromBuffer (buffer.getPointer(pos));
        pos = header.next;
        numMembers++;
    }    
    if (1 == numMembers)
    {
        u8 numDim = buffer.readU8At (pos_dataBlock + 4);
        numDim |= 0x80;
        buffer.writeU8At (pos_dataBlock + 4, numDim);
    }
    
    return *this;
}
