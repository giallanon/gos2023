#include "gosDataBlob.h"
#include "gos.h"

using namespace gos;
using namespace gos::datablob;

//*****************************
void DefElem::priv_setup (const BufferR *readerIN, u16 startingPos, u16 endingPosIN)
{
    reader = readerIN;
    header.decodeFromBuffer (reader->getPointer(startingPos));
    pos_curElem = startingPos;
    endingPos = endingPosIN;
}

//*****************************
bool DefElem::next()
{
    const u16 pos_nextElem = header.next;
    if (pos_nextElem >= endingPos)
        return false;

    pos_curElem = pos_nextElem;
    header.decodeFromBuffer (reader->getPointer(pos_curElem));
    return true;
}

//*****************************
bool DefElem::firstChild()
{
    if (eDataBlobElemType::simpleType == getType())
        return false;

    const u16 posFistChild = reader->readU16At (pos_curElem + header.sizeof_thisHeader);
    const u16 endPosOfLastChild = reader->readU16At (pos_curElem + header.sizeof_thisHeader + 2);
    priv_setup (reader, posFistChild, endPosOfLastChild);
    return true;
}

//*****************************
bool DefElem::hasChild() const
{
    if (eDataBlobElemType::simpleType == getType())
        return false;
    return true;
}

//*****************************
bool DefElem::getFirstChild (DefElem *out) const
{
    assert (NULL != out);
    if (eDataBlobElemType::simpleType == getType())
        return false;

    const u16 posFistChild = reader->readU16At (pos_curElem + header.sizeof_thisHeader);
    const u16 endPosOfLastChild = reader->readU16At (pos_curElem + header.sizeof_thisHeader + 2);
    out->priv_setup (reader, posFistChild, endPosOfLastChild);
    return true;
}

//*****************************
bool DefElem::getNextSibling (DefElem *out) const
{
    assert (NULL != out);
    const u16 pos_nextElem = header.next;
    if (pos_nextElem >= endingPos)
        return false;

    out->priv_setup (reader, pos_nextElem, endingPos);
    return true;
}

//*****************************
eDataFormat DefElem::getDataFmt() const
{
    if (eDataBlobElemType::simpleType == getType())
    {
        const u32 pos_startOfDataBlock = pos_curElem + header.sizeof_thisHeader;
        return static_cast<eDataFormat> (reader->readU8At (pos_startOfDataBlock));
    }
    return eDataFormat::_unknown;
}

//*****************************
u8 DefElem::structType_getNumMembers() const
{
    assert (getType() == eDataBlobElemType::structType);
    const u32 pos_startOfDataBlock = pos_curElem + header.sizeof_thisHeader;

    return reader->readU8At (pos_startOfDataBlock + 4);
}

//*****************************
u8 DefElem::arrayType_getNumDimension() const
{
    assert (getType() == eDataBlobElemType::arrayType);
    const u32 pos_startOfDataBlock = pos_curElem + header.sizeof_thisHeader;

    return (reader->readU8At (pos_startOfDataBlock + 4) & 0x7F);
}

//*****************************
bool DefElem::arrayType_isSimple() const
{
    assert (getType() == eDataBlobElemType::arrayType);
    const u32 pos_startOfDataBlock = pos_curElem + header.sizeof_thisHeader;

    return ((reader->readU8At (pos_startOfDataBlock + 4) & 0x80) != 0);
}

//*****************************
u16 DefElem::arrayType_getNumElem (u8 index) const
{
    assert (getType() == eDataBlobElemType::arrayType);
    assert (index < arrayType_getNumDimension());
    const u32 pos_startOfDataBlock = pos_curElem + header.sizeof_thisHeader;

    return reader->readU16At (pos_startOfDataBlock + 6 + index*2);
}

//*****************************
u8 DefElem::arrayType_getStride() const
{
    assert (getType() == eDataBlobElemType::arrayType);
    const u32 pos_startOfDataBlock = pos_curElem + header.sizeof_thisHeader;

    return reader->readU8At (pos_startOfDataBlock + 5);
}

//*****************************
bool DefReader::setup (const void *dataBlobDef)
{
    //verifico che il blob sia in effetti una DataBlobDef
    if (!datablob::blobDef_isValidMagic(dataBlobDef))
        return false;

    //preparo il reader
    const u16 sizeof_blobDef = datablob::blobDef_getSize(dataBlobDef);
    reader.setup (dataBlobDef, sizeof_blobDef, eEndianess::big);
    return true;
}

//*****************************
void DefReader::beginEnumerate (DefElem *out) const
{
    assert (NULL != out);
    const u16 sizeof_blobDef = reader.readU16At (4);
    out->priv_setup (&reader, 8, sizeof_blobDef);
}

//*****************************
bool DefReader::getOffset (const char *var_name, u16 *out) const
{
    assert (NULL != out);
    if (NULL == var_name)
        return false;

    string::utf8::StringListParser parser;
    parser.toStart (var_name, '.');

    DefElem elem;
    beginEnumerate (&elem);

    char s[128];
    if (!parser.next(s, sizeof(s)))
        return false;

    bool bEsci = false;
    u16 array_offset = 0;
    while (false == bEsci)
    {
        //cerco di capire se <s> e' un nome di array e, se si, valorizzo
        //array_index[] con il numero/i numeri riportato/i tra parentesi quadre
        u8  array_ordine = 0;
        u16 array_index[8];
        u32 len = strlen(s);
        if (0 == len)
            return false;
        if (s[len-1] == ']')
        {
            //ho trovato una [,il che vuol dire che <s> sta indicando un array.
            //Essendo che l'array puo' essere multidimensionale, devo recuperare
            //l'indice per ogni dimensione
            u32 i = 0;
            while (i<len)
            {
                if (s[i] != '[')
                {
                    i++;
                    continue;
                }

                //ho trovato la prima [
                //Da qui in poi deve essere una sequenza di [num][num]..[num]
                while (i<len)
                {
                    if (s[i] != '[')
                        return false;

                    s[i++] = 0x00;
                    const u8 pos = i;
                    while (i<len)
                    {
                        if (s[i] == ']')
                        {
                            s[i++] = 0x00;
                            const u32 index = gos::string::ansi::toI32(&s[pos]);
                            if (index < 0)
                                return false;
                            if (index >= 65535)
                                return false;

                            array_index[array_ordine++] = static_cast<u16>(index);
                            break;
                        }
                        i++;
                    }
                }
            }

            if (array_ordine == 0)
                return false;
        }

        //cerco <s> tra i fratelli di <elem>
        bEsci = true;
        do
        {
            if (0 == strcmp(s, elem.getName()))
            {
                //ho trovato un nome che matcha.
                //Se stavo indicizzando un array, verifico gli indici
                if (0 != array_ordine)
                {
                    if (eDataBlobElemType::arrayType != elem.getType())
                        return false;
                    if (elem.arrayType_getNumDimension() != array_ordine)
                        return false;
                    
                    const u16 n = array_ordine-1;
                    u16 offset = 0;
                    for (u8 i=0; i<n; i++)
                    {
                        if (array_index[i] >= elem.arrayType_getNumElem(i))
                            return false;
                        offset += array_index[i] * elem.arrayType_getNumElem(i+1);
                    }
                    if (array_index[n] >= elem.arrayType_getNumElem(n))
                        return false;
                    offset += array_index[n];

                    array_offset += offset * elem.arrayType_getStride();
                }

                //se siamo a fine <var_name>...
                if (!parser.next(s, sizeof(s)))
                {
                    *out = elem.getOffset() + array_offset;
                    return true;
                }

                //c'e' un altro pezzo di var name da analizzare
                elem.firstChild();
                bEsci = false;
                break;
            }
        } while (elem.next());
    }
    return false;
}