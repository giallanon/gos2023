#ifndef _gosDataFormat_h_
#define _gosDataFormat_h_
#include "gosDataTypes.h"

/* eDataFormat e' un byte i cui singoli bit hanno i seguenti valori:
 *
 *  00000000
 *  ------aa    => 2 bit per indicare il tipo
 *  -----b--    => 1 bit per indicare se signed o unsigned
 *  ---cc---    => 2 bit per indicare il num di elementi (0=1 elemento, 1=2 elementi..)
 *  mmm-----    => determinano il significato dei bit che seguono
 * 
 *      1rrccbaa    => matrix [rr+1] [cc+1] elementi di tipo [aa] e signed dipende da [b]
 *      
 *      010ccbaa    => array [cc+1] elementi di tipo [aa] e signed dipende da [b]
 *      011ccbaa    => array [cc+1] elementi di tipo [aa] UNORM e signed dipende da [b] 
 *  
 *      00011111    => unknown format
 */
#define GOS_DATAFMT__TYPE_8bit      0b00000000
#define GOS_DATAFMT__TYPE_16bit     0b00000001
#define GOS_DATAFMT__TYPE_32bit     0b00000010
#define GOS_DATAFMT__TYPE_F32       0b00000011

#define GOS_DATAFMT__SIGNED         0b00000100

#define GOS_DATAFMT__NUM_ELEM_1     0b00000000
#define GOS_DATAFMT__NUM_ELEM_2     0b00001000
#define GOS_DATAFMT__NUM_ELEM_3     0b00010000
#define GOS_DATAFMT__NUM_ELEM_4     0b00011000

#define GOS_DATAFMT__IS_MATRIX      0b10000000
#define GOS_DATAFMT__IS_ARRAY       0b01000000
#define GOS_DATAFMT__IS_ARRAY_UNORM 0b01100000

#define GOS_DATAFMT__MATRIX_1ROW    0b10000000
#define GOS_DATAFMT__MATRIX_2ROW    0b10100000
#define GOS_DATAFMT__MATRIX_3ROW    0b11000000
#define GOS_DATAFMT__MATRIX_4ROW    0b11100000

#define GOS_DATAFMT__UNKNOWN        0b00011111

static constexpr u8 GOS_DATAFMT_1f32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_F32 | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_1;
static constexpr u8 GOS_DATAFMT_2f32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_F32 | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_2;
static constexpr u8 GOS_DATAFMT_3f32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_F32 | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_3;
static constexpr u8 GOS_DATAFMT_4f32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_F32 | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_4;

static constexpr u8 GOS_DATAFMT_1u32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_32bit | GOS_DATAFMT__NUM_ELEM_1;
static constexpr u8 GOS_DATAFMT_2u32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_32bit | GOS_DATAFMT__NUM_ELEM_2;
static constexpr u8 GOS_DATAFMT_3u32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_32bit | GOS_DATAFMT__NUM_ELEM_3;
static constexpr u8 GOS_DATAFMT_4u32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_32bit | GOS_DATAFMT__NUM_ELEM_4;

static constexpr u8 GOS_DATAFMT_1i32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_32bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_1;
static constexpr u8 GOS_DATAFMT_2i32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_32bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_2;
static constexpr u8 GOS_DATAFMT_3i32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_32bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_3;
static constexpr u8 GOS_DATAFMT_4i32 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_32bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_4;

static constexpr u8 GOS_DATAFMT_1u16 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_16bit | GOS_DATAFMT__NUM_ELEM_1;
static constexpr u8 GOS_DATAFMT_2u16 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_16bit | GOS_DATAFMT__NUM_ELEM_2;
static constexpr u8 GOS_DATAFMT_3u16 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_16bit | GOS_DATAFMT__NUM_ELEM_3;
static constexpr u8 GOS_DATAFMT_4u16 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_16bit | GOS_DATAFMT__NUM_ELEM_4;

static constexpr u8 GOS_DATAFMT_1i16 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_16bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_1;
static constexpr u8 GOS_DATAFMT_2i16 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_16bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_2;
static constexpr u8 GOS_DATAFMT_3i16 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_16bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_3;
static constexpr u8 GOS_DATAFMT_4i16 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_16bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_4;

static constexpr u8 GOS_DATAFMT_1u8 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_8bit | GOS_DATAFMT__NUM_ELEM_1;
static constexpr u8 GOS_DATAFMT_2u8 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_8bit | GOS_DATAFMT__NUM_ELEM_2;
static constexpr u8 GOS_DATAFMT_3u8 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_8bit | GOS_DATAFMT__NUM_ELEM_3;
static constexpr u8 GOS_DATAFMT_4u8 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_8bit | GOS_DATAFMT__NUM_ELEM_4;
static constexpr u8 GOS_DATAFMT_4u8_UNORM = GOS_DATAFMT__IS_ARRAY_UNORM | GOS_DATAFMT__TYPE_8bit | GOS_DATAFMT__NUM_ELEM_4;

static constexpr u8 GOS_DATAFMT_1i8 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_8bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_1;
static constexpr u8 GOS_DATAFMT_2i8 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_8bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_2;
static constexpr u8 GOS_DATAFMT_3i8 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_8bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_3;
static constexpr u8 GOS_DATAFMT_4i8 = GOS_DATAFMT__IS_ARRAY | GOS_DATAFMT__TYPE_8bit | GOS_DATAFMT__SIGNED | GOS_DATAFMT__NUM_ELEM_4;

static constexpr u8 FOS_DATAFMT_MAT2x2 = GOS_DATAFMT__IS_MATRIX | GOS_DATAFMT__TYPE_F32 | GOS_DATAFMT__SIGNED | GOS_DATAFMT__MATRIX_2ROW | GOS_DATAFMT__NUM_ELEM_2;
static constexpr u8 FOS_DATAFMT_MAT3x3 = GOS_DATAFMT__IS_MATRIX | GOS_DATAFMT__TYPE_F32 | GOS_DATAFMT__SIGNED | GOS_DATAFMT__MATRIX_3ROW | GOS_DATAFMT__NUM_ELEM_3;
static constexpr u8 FOS_DATAFMT_MAT4x4 = GOS_DATAFMT__IS_MATRIX | GOS_DATAFMT__TYPE_F32 | GOS_DATAFMT__SIGNED | GOS_DATAFMT__MATRIX_4ROW | GOS_DATAFMT__NUM_ELEM_4;

enum class eDataFormat_type : u8
{
    _8bit   = GOS_DATAFMT__TYPE_8bit,
    _16bit  = GOS_DATAFMT__TYPE_16bit,
    _32bit  = GOS_DATAFMT__TYPE_32bit,
    _f32    = GOS_DATAFMT__TYPE_F32
};

enum class eDataFormat : u8
{
    _unknown        = GOS_DATAFMT__UNKNOWN,
    _1f32			= GOS_DATAFMT_1f32,
    _2f32			= GOS_DATAFMT_2f32,
    _3f32			= GOS_DATAFMT_3f32,
    _4f32			= GOS_DATAFMT_4f32,

    _1u32			= GOS_DATAFMT_1u32,
    _2u32			= GOS_DATAFMT_2u32,
    _3u32			= GOS_DATAFMT_3u32,
    _4u32			= GOS_DATAFMT_4u32,

    _1i32			= GOS_DATAFMT_1i32,
    _2i32			= GOS_DATAFMT_2i32,
    _3i32			= GOS_DATAFMT_3i32,
    _4i32			= GOS_DATAFMT_4i32,

    _1u16			= GOS_DATAFMT_1u16,
    _2u16			= GOS_DATAFMT_2u16,
    _3u16			= GOS_DATAFMT_3u16,
    _4u16			= GOS_DATAFMT_4u16,

    _1i16			= GOS_DATAFMT_1i16,
    _2i16			= GOS_DATAFMT_2i16,
    _3i16			= GOS_DATAFMT_3i16,
    _4i16			= GOS_DATAFMT_4i16,    

    _1u8			= GOS_DATAFMT_1u8,
    _2u8			= GOS_DATAFMT_2u8,
    _3u8			= GOS_DATAFMT_3u8,
    _4u8			= GOS_DATAFMT_4u8,
    _4u8_norm       = GOS_DATAFMT_4u8_UNORM,

    _1i8			= GOS_DATAFMT_1i8,
    _2i8			= GOS_DATAFMT_2i8,
    _3i8			= GOS_DATAFMT_3i8,
    _4i8			= GOS_DATAFMT_4i8,

    _mat2x2         = FOS_DATAFMT_MAT2x2,   //si intende una matrice di float
    _mat3x3         = FOS_DATAFMT_MAT3x3,
    _mat4x4         = FOS_DATAFMT_MAT4x4    
};

namespace gos
{
    namespace dataformat
    {
        u8                  getSize (eDataFormat f);

        eDataFormat_type    getBasicType (eDataFormat f);
        bool                isSigned (eDataFormat f);
        bool                isUnsigned (eDataFormat f);
        bool                isArray (eDataFormat f);
        bool                isArrayUNORM (eDataFormat f);
        bool                isMatrix (eDataFormat f);
        u8                  getArrayNumElem (eDataFormat f);
        u8                  getMatrixNumRow (eDataFormat f);
        u8                  getMatrixNumCol (eDataFormat f);

        eDataFormat         build (eDataFormat_type type, bool bSigned, u8 numRow, u8 numCol);

    } //namespace dataformat
} //namespace gos

#endif // _gosDataFormat_h_

