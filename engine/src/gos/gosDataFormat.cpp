#include "gosDataFormat.h"


//*****************************************
u8 gos::dataformat::getSize (eDataFormat f)
{
    u8 numElem;
    if (dataformat::isMatrix(f))
    {
        numElem = dataformat::getMatrixNumCol(f) * dataformat::getMatrixNumRow(f);
    }
    else
    {
        numElem = dataformat::getArrayNumElem(f);
    }

    switch (dataformat::getBasicType(f))
    {
    default:
        DBGBREAK;
        return 0;

    case eDataFormat_type::_8bit:   return numElem;
    case eDataFormat_type::_16bit:  return numElem*2;
    case eDataFormat_type::_32bit:  return numElem*4;
    case eDataFormat_type::_f32:    return numElem*4;
    }
}

//*****************************************
eDataFormat_type gos::dataformat::getBasicType (eDataFormat f)
{
    return static_cast<eDataFormat_type> ( (static_cast<u8>(f) & 0x03) );
}

//*****************************************
bool gos::dataformat::isSigned (eDataFormat f)
{
    return ((static_cast<u8>(f) & GOS_DATAFMT__SIGNED) != 0);
}

//*****************************************
bool gos::dataformat::isUnsigned (eDataFormat f)
{
    return ((static_cast<u8>(f) & GOS_DATAFMT__SIGNED) == 0);
}

//*****************************************
bool gos::dataformat::isMatrix (eDataFormat f)
{
    return ( (static_cast<u8>(f) & 0b10000000) == GOS_DATAFMT__IS_MATRIX );
}

//*****************************************
bool gos::dataformat::isArray (eDataFormat f)
{
    return ( (static_cast<u8>(f) & 0b11000000) == GOS_DATAFMT__IS_ARRAY );
}
//*****************************************
bool gos::dataformat::isArrayUNORM (eDataFormat f)
{
    return ((static_cast<u8>(f) & 0b11100000) == GOS_DATAFMT__IS_ARRAY_UNORM);
}

//*****************************************
u8 gos::dataformat::getArrayNumElem (eDataFormat f)
{
    assert (dataformat::isArray(f));
    return 1 + ((static_cast<u8>(f) & 0b00011000) >> 3);
}

//*****************************************
u8 gos::dataformat::getMatrixNumRow (eDataFormat f)
{
    assert (dataformat::isMatrix(f));
    return 1 + ((static_cast<u8>(f) & 0b01100000) >> 5);
}

//*****************************************
u8 gos::dataformat::getMatrixNumCol (eDataFormat f)
{
    assert (dataformat::isMatrix(f));
    return 1 + ((static_cast<u8>(f) & 0b00011000) >> 3);
}

//*****************************************
eDataFormat gos::dataformat::build (eDataFormat_type type, bool bSigned, u8 numRow, u8 numCol)
{
    assert (numRow <= 4);
    assert (numCol <= 4);
    assert (numCol > 0);

    u8 ret = 0;
    switch (numRow)
    {
    default:    DBGBREAK; return eDataFormat::_unknown;
    case 0:     ret = GOS_DATAFMT__IS_ARRAY; break;
    case 1:     ret = GOS_DATAFMT__MATRIX_1ROW; break;
    case 2:     ret = GOS_DATAFMT__MATRIX_2ROW; break;
    case 3:     ret = GOS_DATAFMT__MATRIX_3ROW; break;
    case 4:     ret = GOS_DATAFMT__MATRIX_4ROW; break;
    }

    if (bSigned)
        ret |= GOS_DATAFMT__SIGNED;

    ret |= static_cast<u8>(type);

    switch (numCol)
    {
    default:    DBGBREAK; return eDataFormat::_unknown;
    case 1:     ret |= GOS_DATAFMT__NUM_ELEM_1; break;
    case 2:     ret |= GOS_DATAFMT__NUM_ELEM_2; break;
    case 3:     ret |= GOS_DATAFMT__NUM_ELEM_3; break;
    case 4:     ret |= GOS_DATAFMT__NUM_ELEM_4; break;
    }

    return static_cast<eDataFormat>(ret);
}