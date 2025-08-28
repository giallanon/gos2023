#ifndef _gosAssetEnumAndDefine_h_
#define _gosAssetEnumAndDefine_h_
#include "gosEnumAndDefine.h"
#include "gosHashMap.h"
#include "gosDB.h"

namespace gos
{
    enum class eAssetType : u8
    {
        __DO__NOT__USE  = 0,
        vtx_shader      = 1,
        pxl_shader      = 2,
        pipeline_def    = 3,
        texture2D       = 4,
        shape           = 5,
    };

    namespace asset
    {
        enum class eResType : u8
        {
            __DO__NOT__USE  = 0,
            gosasset_d      = 1,
            shader_txt      = 2,
            image           = 3,
            
            __FINISHED      = 4 //questo deve sempre essere uguale al valore dell'ultimo enum + 1
                                //E' importante mantenere enumToString() coerente con questa enum
        };


        enum class eBuildResult : u8
        {
            just_built          = 0,
            was_already_built   = 1,
            error               = 0xff
        };
        
        struct UID
        {
            u64 _uid;

        public:
            void    setInvalid()                                            { _uid=0; }
            bool    isValid() const                                         { return (_uid != 0); }

            bool    isAResource() const                                     { return (((_uid >> 40) & 0xFF) != 0); }
            bool    isAnAsset() const                                       { return (((_uid >> 48) & 0xFF) != 0); }

            int     compare (const UID &b) const                            { if (_uid == b._uid) return 0; if (_uid > b._uid) return 1; return -1; }
            bool    operator== (const asset::UID &b) const                  { return _uid == b._uid; }
            bool    operator!= (const asset::UID &b) const                  { return _uid != b._uid; }
            bool    operator>  (const asset::UID &b) const                  { return _uid > b._uid; }
            bool    operator<  (const asset::UID &b) const                  { return _uid < b._uid; }
            bool    operator>= (const asset::UID &b) const                  { return _uid >= b._uid; }
            bool    operator<= (const asset::UID &b) const                  { return _uid <= b._uid; }

            void    operator= (u64 i)                                       { priv_setFromU64(i); }


        private:
            void    priv_setFromU64 (u64 i)
                    {
                        _uid = static_cast<u64>(i);
                        #ifdef _DEBUG
                            //sqlite limita gli int64 tra -9223372036854775808 and +9223372036854775807 
                            if (_uid >= 9223372036854775807)
                                DBGBREAK;
                        #endif
                    }            
        };


        typedef gos::HashMap<asset::UID, u64>  HashedUIDList;


        struct Context
        {
        public:
                        Context()           { baseFolder=folder_assets_src=folder_assets_bin=folder_res=NULL; }
            bool        isValid() const     { return (baseFolder != NULL); }

        public:
            char        *baseFolder;
            char        *folder_assets_src;
            char        *folder_assets_bin;
            char        *folder_res;
            DBHandle    db;
        };
        

        struct sBuildResult
        {
        public:
            void            reset()     { uid=0; result=eBuildResult::error; }
        
        public:
            asset::UID      uid;
            eBuildResult    result;
        };        

    } //namespace asset

} //namespace gos

#endif //_gosAssetEnumAndDefine_h_