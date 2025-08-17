#ifndef _SPVDataType_h_
#define _SPVDataType_h_
#include "gos.h"
#include "gosBufferWriter.h"
#include "gosLIFOFixedSize.h"


class SPVDataTypeDefinition
{
public:
            SPVDataTypeDefinition();
            ~SPVDataTypeDefinition();

    void    begin();

    void    add_simple (const char *var_name, eDataFormat fmt, u32 paddedSize = u32MAX);

    void    begin_struct (const char *var_name);
            //add_simple...
            //add_simple...
            //...
    void    end_struct();

    void    begin_array (const char *var_name, u16 numElem);
            //add_simple...
            //add_simple...
            //...
    void    end_array ();


    void    end();



    void    debug_print_just_names() const;

private:
    static constexpr u8 MAX_NAME_SIZE = 32;

    static constexpr u8 WHAT__NEW_SIMPLE  = 0x01;
    static constexpr u8 WHAT__NEW_STRUCT  = 0x02;
    static constexpr u8 WHAT__NEW_ARRAY   = 0x03;

private:
    struct sHeaderInfo
    {
        char        name[MAX_NAME_SIZE];
        u32         headerSize;
        u32         sizeOfDataBlock;
        u8          what;
        u8          nameLen;
    };


private:
    u32     priv_add_header (u8 what, const char *var_name);
    void    priv_add_footer (u32 pos_of_header);
    void    priv_read_header (u32 pos_of_header, sHeaderInfo *out) const;

    u32     debug_priv_print_block (u32 pos_of_header) const;
    u32     debug_priv_print_block_0x01 (u32 pos_of_header) const;
    u32     debug_priv_print_block_0x02 (u32 pos_of_header) const;
    u32     debug_priv_print_block_0x03 (u32 pos_of_header) const;

private:
    gos::LIFOFixedSize<u32, 16> stack;
    gos::BufferW_linear         buffer;
    

}; // SPVType

#endif //_SPVType_h_