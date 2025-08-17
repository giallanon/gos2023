#include "SPVDataType.h"

//********************************** 
SPVDataTypeDefinition::SPVDataTypeDefinition()
{
    buffer.setup (gos::getSysHeapAllocator(), 256, eEndianess::big);
    begin();
}

//********************************** 
SPVDataTypeDefinition::~SPVDataTypeDefinition()
{
    buffer.unsetup();
}

//********************************** 
void SPVDataTypeDefinition::begin()
{
    buffer.reset();
    stack.reset();
}

//********************************** 
void SPVDataTypeDefinition::end()
{
}

//********************************** 
u32 SPVDataTypeDefinition::priv_add_header (u8 what, const char *var_name)
{
    const u32 pos_of_header = buffer.tell();
    buffer.writeU8 (what);

    //u16 sizeOfDataBlock
    u16 sizeOfDataBlock = 0;
    buffer.writeU16 (sizeOfDataBlock);

    //nameLen + name (include lo 0x00 finale)
    const u8 nameLen = static_cast<u8> (1 + strlen(var_name));
    buffer.writeU8  (nameLen);
    buffer.write (var_name, nameLen);

    return pos_of_header;
}

//********************************** 
void SPVDataTypeDefinition::priv_read_header (u32 pos, sHeaderInfo *out) const
{
    assert (NULL != out);

    const u32 blockStartAt = pos;

    out->what = buffer.readU8At (pos);
    pos++;
    
    out->sizeOfDataBlock = buffer.readU16At (pos);
    pos += 2;

    out->nameLen = buffer.readU8At (pos);
    pos++;

    memset (out->name, 0, MAX_NAME_SIZE);
    buffer.readAtAndCopyHere (pos, out->name, out->nameLen);
    pos += out->nameLen;

    out->headerSize = pos - blockStartAt;

}

//********************************** 
void SPVDataTypeDefinition::priv_add_footer (u32 pos_of_header)
{
    const u16 sizeOfDataBlock = static_cast<u16> (buffer.tell() - pos_of_header - 1);
    buffer.writeU16At (pos_of_header+1, sizeOfDataBlock);
}

//********************************** 
void SPVDataTypeDefinition::add_simple (const char *var_name, eDataFormat fmt, u32 paddedSize)
{
    const u32 pos_of_header = priv_add_header (WHAT__NEW_SIMPLE, var_name);

    //dataftm
    buffer.writeU8 (static_cast<u8>(fmt));

    //paddedSize
    if (u32MAX == paddedSize)
        paddedSize = gos::dataformat::getSize(fmt);
    buffer.writeU32 (paddedSize);

    priv_add_footer (pos_of_header);
 
}

//********************************** 
void SPVDataTypeDefinition::begin_struct (const char *var_name)
{
    const u32 pos_of_header = priv_add_header (WHAT__NEW_STRUCT, var_name);
    stack.push (pos_of_header);

    //num members
    buffer.writeU8 (0);
}

//********************************** 
void SPVDataTypeDefinition::end_struct()
{
    u32 pos_of_header;
    stack.pop (&pos_of_header);
    priv_add_footer (pos_of_header);

    //conta il num di membri della struct
    sHeaderInfo header;
    priv_read_header (pos_of_header, &header);
    assert (WHAT__NEW_STRUCT == header.what);

    u8 numMembers = 0;
    const u32 pos_of_num_members = pos_of_header + header.headerSize;
    u32 pos = pos_of_num_members+1;

    const u32 pos_final = buffer.tell();
    while (pos < pos_final)
    {
        sHeaderInfo header;
        priv_read_header (pos, &header);
        pos = pos + header.sizeOfDataBlock +1;
        numMembers++;
    }

    buffer.writeU8At (pos_of_num_members, numMembers);
}

//********************************** 
void SPVDataTypeDefinition::begin_array (const char *var_name, u16 numElem)
{
    const u32 pos_of_header = priv_add_header (WHAT__NEW_ARRAY, var_name);
    stack.push (pos_of_header);

    //numElem
    buffer.writeU16 (numElem);
}


//********************************** 
void SPVDataTypeDefinition::end_array ()
{
    u32 pos_of_header;
    stack.pop (&pos_of_header);
    priv_add_footer (pos_of_header);
}


//********************************** 
u32 SPVDataTypeDefinition::debug_priv_print_block_0x01 (u32 pos) const
{
    const u32 blockStartAt = pos;

    sHeaderInfo header;
    priv_read_header (pos, &header);
    assert (WHAT__NEW_SIMPLE == header.what);

    pos += header.headerSize;

    const eDataFormat fmt = static_cast<eDataFormat> (buffer.readU8At (pos));
    pos++;

    const u32 paddedSize = buffer.readU32At (pos);



    gos::logger::log ("SIMPLE,  pos=% 4d, name=%-16s, fmt=%-8s, padded-size=% 3d\n", 
                    blockStartAt, 
                    header.name,
                    gos::utils::enumToString (fmt),
                    paddedSize
                    );


    return blockStartAt + header.sizeOfDataBlock +1;
}

//********************************** 
u32 SPVDataTypeDefinition::debug_priv_print_block_0x02 (u32 pos) const
{
    const u32 blockStartAt = pos;

    sHeaderInfo header;
    priv_read_header (pos, &header);
    assert (WHAT__NEW_STRUCT == header.what);

    pos += header.headerSize;
    const u8 numMembers = buffer.readU8At(pos);
    pos++;

    gos::logger::log ("STRUCT,  pos=% 4d, name=%-16s, num-members=%d\n", blockStartAt, header.name, numMembers);
    gos::logger::incIndent();

    const u32 pos_final = blockStartAt + header.sizeOfDataBlock +1;
    while (pos < pos_final)
    {
        pos = debug_priv_print_block (pos);
    }

    gos::logger::decIndent();

    return pos_final;
}

//********************************** 
u32 SPVDataTypeDefinition::debug_priv_print_block_0x03 (u32 pos) const
{
    const u32 blockStartAt = pos;

    sHeaderInfo header;
    priv_read_header (pos, &header);
    assert (WHAT__NEW_ARRAY == header.what);

    pos += header.headerSize;
    const u16 numElem = buffer.readU16At(pos);
    pos+=2;

    gos::logger::log ("ARRAY,  pos=% 4d, name=%-16s, numElem=%d\n", blockStartAt, header.name, numElem);
    gos::logger::incIndent();

    const u32 pos_final = blockStartAt + header.sizeOfDataBlock +1;
    while (pos < pos_final)
    {
        pos = debug_priv_print_block (pos);
    }

    gos::logger::decIndent();

    return pos_final;
}

//********************************** 
u32 SPVDataTypeDefinition::debug_priv_print_block (u32 pos_of_header) const
{
    u32 new_pos;
    switch (buffer.readU8At (pos_of_header))
    {
    default:
        DBGBREAK;
        new_pos = pos_of_header +1;
        break;

    case WHAT__NEW_SIMPLE:
        new_pos = debug_priv_print_block_0x01 (pos_of_header);
        break;

    case WHAT__NEW_STRUCT:
        new_pos = debug_priv_print_block_0x02 (pos_of_header);
        break;   

    case WHAT__NEW_ARRAY:
        new_pos = debug_priv_print_block_0x03 (pos_of_header);
        break;              
    }

    return new_pos;
}

//********************************** 
void SPVDataTypeDefinition::debug_print_just_names() const
{
    u32 pos = 0;

    while (pos < buffer.tell())
    {
        pos = debug_priv_print_block (pos);
    }
}