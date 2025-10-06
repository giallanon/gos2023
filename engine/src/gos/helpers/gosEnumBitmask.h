#ifndef _gosEnumBitmask_h_
#define _gosEnumBitmask_h_
#include <type_traits>

/**
 * la macro GOS_DECL_ENUM_BITMASK_CLASS() definisce una struct di comodo per utilizzare gli enum a mo' di bitmask.
 * 
 * Data una enum del tipo
 * enum class ePippo : u8
 * {
 *  none = 0,
 *  bt1 = 0x01,
 *  bit2 = 0x02
 * }
 * 
 * allora
 *  GOS_DECL_ENUM_BITMASK_CLASS(ePippo)
 * dichiara una struct di nome
 *  ePippoBitmask
 * sulla quale e' possibile usare ePippo come una bitmask con operazioni del tipo:
 *      ePippoBitmask mask;
 *      mask = ePippo::bit1 | ePippo::bit2;
 *      mask |= ePippo::bit1;
 * 
*/


template<class TENUM>
struct EnumBitmask_TPL   
{   
private:   
    using MyType = EnumBitmask_TPL<TENUM>;   
    using MASK_TYPE = std::underlying_type_t<TENUM>;   
public:   
                EnumBitmask_TPL ()                          { bitmask = 0; }   
                EnumBitmask_TPL (const TENUM b)             { bitmask = static_cast<MASK_TYPE>(b); }   
    void        zero()                                      { bitmask = 0; }   
    void        bitset(const TENUM b)                       { bitmask |= static_cast<MASK_TYPE>(b); }   
    void        bitclear(const TENUM b)                     { bitmask &= ~static_cast<MASK_TYPE>(b); }   
    bool        isset(const TENUM b) const                  { return (bitmask & static_cast<MASK_TYPE>(b)) != 0; }   
    MyType&     operator=  (const TENUM b)                  { bitmask = static_cast<MASK_TYPE>(b); return *this; }   
    MyType&     operator|= (const TENUM b)                  { bitmask |= static_cast<MASK_TYPE>(b); return *this; }   
    MyType&     operator&= (const TENUM b)                  { bitmask &= static_cast<MASK_TYPE>(b); return *this; }   
    MyType&     operator| (const TENUM b)                   { bitmask |= static_cast<MASK_TYPE>(b); return *this; }   
    MyType&     operator& (const TENUM b)                   { bitmask &= static_cast<MASK_TYPE>(b); return *this; }   

    void        beginFetch (u8 *iter) const                 { *iter = 0; }   
    bool        fetch (u8 &iter, TENUM *out) const   
    {   
        MASK_TYPE mask = 1 << iter;   
        while (iter < sizeof(MASK_TYPE)*8 )   
        {   
            if ((bitmask & mask) != 0)   
            {   
                *out = static_cast<TENUM>(mask);   
                iter++;   
                return true;   
            }   
            iter++;   
            mask<<=1;   
        }   
        return false;   
    }   

public:   
    MASK_TYPE   bitmask;   
};   



#define GOS_DECL_ENUM_BITMASK_CLASS(TENUM)\
    \
    typedef EnumBitmask_TPL<TENUM>   TENUM##Bitmask; \
    \
    inline TENUM##Bitmask  operator| (const TENUM a, const TENUM b)   { TENUM##Bitmask ret(a); ret|=b; return ret; }   \




#endif //_gosEnumBitmask_h_