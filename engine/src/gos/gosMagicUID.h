#ifndef _gosMagicUID_h_
#define _gosMagicUID_h_

/**
 * @brief e' un elenco di tutti i MAGIC utilizzati nell'intero progetto
 * 
 */

namespace gos
{
    namespace magic
    {
        inline constexpr u32 _makeID (u32 signature_24bit, u32 version_8bit)        { return signature_24bit | (version_8bit << 24); }
        
        inline constexpr u32 getVer (u32 magic)                                     { return ((magic >> 24) & 0xFF); }
        inline bool signatureMatch (u32 magicA, u32 magicB)                         { return ((magicA & 0x00FFFFFF) == (magicB & 0x00FFFFFF)); }
        inline bool versionMatch (u32 magicA, u32 magicB)                           { return ((magicA & 0xFF000000) == (magicB & 0xFF000000)); }
    } //namespace magic
} //namespace gos



//i magic definiti nel modulo gos iniziano da 0xA10000
static constexpr u32 GOS_MAGIC__DATA_BLOB_DEF       = gos::magic::_makeID (0xA10000, 0x01);

//i magic definiti nel modulo gos::Engine con 0xA20000
static constexpr u32 GOS_MAGIC__ENGINE_SKELETON      = gos::magic::_makeID (0xA20000, 0x01);


//i magic definiti nel modulo gos::shape con 0xA70000
static constexpr u32 GOS_MAGIC__SHAPE               = gos::magic::_makeID (0xA70000, 0x01);
static constexpr u32 GOS_MAGIC__VTX_LAYOUT          = gos::magic::_makeID (0xA70001, 0x01);

//i magic definiti nel modulo gos::GPU con 0xA80000
//static constexpr u32 GOS_MAGIC__GPU_FRAME_BUFFER_DEF    = gos::magic::_makeID (0xA80000, 0x01);


//i magic definiti nel modulo gos::Asset con 0xA90000
static constexpr u32 GOS_MAGIC__ASSET_PIPELINE_DEF      = gos::magic::_makeID (0xA90000, 0x04);




#endif //_gosMagicUID_h_