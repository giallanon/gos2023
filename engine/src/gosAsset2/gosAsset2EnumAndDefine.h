#ifndef _gosAsset2EnumAndDefine_h_
#define _gosAsset2EnumAndDefine_h_
#include "gosEnumAndDefine.h"
#include "gosHashMap.h"
#include "gosUniqueSortedList.h"
#include "../gosDB/gosDB.h"

namespace gos
{
    enum class eAssetType : u8
    {
        __DO__NOT__USE  = 0,
        vtx_shader      = 1,
        pxl_shader      = 2,
        pipe            = 3,
        tex2D           = 4,
        shape           = 5,
        imported_glb    = 6,
		skeleton    	= 7,
		model3d			= 8,
		model3dinst		= 9,

        __NUM           = 9 //questo deve essere sempre uguale all'id + alto
    };

    namespace asset2
    {
        enum class eResType : u8
        {
            __DO__NOT__USE  = 0,
            gosasset_d      = 1,
            shader_txt      = 2,
            image           = 3,
            model_glb       = 4,
            
            __FINISHED      = 5 //questo deve sempre essere uguale al valore dell'ultimo enum + 1
                                //E' importante mantenere enumToString() coerente con questa enum
        };

        enum class eBuildResult : u8
        {
            just_built          = 0,
            was_already_built   = 1,
            error               = 0xff
        };        

        /******************
         * @brief   UID
         *          E' logicamente composto da 2 u32
         *          La parte bassa e' un CRC32 che dipende dai parametri di build, oppure un hash del name nel caso di risorse pure
         *          La parte alta:
         *              0x00            => posso usare solo 7 bit per via della limitazione dovuta a sqllite che tratta tutto come signed integer
         *                  bit 0x01    => 0 normalmente, 1 se si tratta di virtual asset
         *              eAssetType      => se identifica un asset, allora questo byte != 0
         *              eResouceType    => se identifica una risorsa, allora questo byte != 0 ed e' di tipo eResType
         */
        struct UID
        {
            u64 _uid;

        public:
            void        setInvalid()                                            { _uid=0; }
            bool        isValid() const                                         { return (_uid != 0); }

            bool        isAResource() const                                     { return ( priv_extractResourceType() != 0); }
            eResType    getResourceType() const                                 { return static_cast<eResType>(priv_extractResourceType()); }
            bool        isAResourceOfType(eResType s) const                     { return (getResourceType() == s); }

            bool        isAnAsset() const                                       { return ( !isVirtualAsset() && priv_extractAssetType() != 0); }
            eAssetType  getAssetType() const                                    { assert(!isVirtualAsset()); return static_cast<eAssetType>(priv_extractAssetType()); }
            bool        isAnAssetOfType(eAssetType s) const                     { assert(!isVirtualAsset()); return (getAssetType() == s); }

            bool        isVirtualAsset() const                                  { return ((_uid & 0x0100000000000000) != 0); }
            eAssetType  getVirtualAssetType() const                             { assert(isVirtualAsset()); return static_cast<eAssetType>(priv_extractAssetType()); }
            bool        isAVirtualAssetOfType(eAssetType s) const               { assert(isVirtualAsset()); return (getAssetType() == s); }

            int         compare (const UID &b) const                            { if (_uid == b._uid) return 0; if (_uid > b._uid) return 1; return -1; }
            bool        operator== (const asset2::UID &b) const                 { return _uid == b._uid; }
            bool        operator!= (const asset2::UID &b) const                 { return _uid != b._uid; }
            bool        operator>  (const asset2::UID &b) const                 { return _uid > b._uid; }
            bool        operator<  (const asset2::UID &b) const                 { return _uid < b._uid; }
            bool        operator>= (const asset2::UID &b) const                 { return _uid >= b._uid; }
            bool        operator<= (const asset2::UID &b) const                 { return _uid <= b._uid; }

        private:
            u8          priv_extractResourceType() const                        { return static_cast<u8>((_uid >> 40) & 0xFF); }
            u8          priv_extractAssetType() const                           { return static_cast<u8>((_uid >> 48) & 0xFF); }
            void        priv_setFromU64 (u64 i)
                    {
                        _uid = static_cast<u64>(i);
                        #ifdef _DEBUG
                            //sqlite limita gli int64 tra -9223372036854775808 and +9223372036854775807 
                            if (_uid >= 9223372036854775807)
                                DBGBREAK;
                        #endif
                    }            
        };


        typedef gos::UniqueSortedList<asset2::UID>  UniqueUIDList;
        typedef gos::FastArray<asset2::UID>         FastUIDList;

		/*************************************
		* @brief    DBContext
		*/
		struct DBContext
		{
		public:
						DBContext()							{ baseFolder = dbName = folder_assets_bin = folder_assets_src = NULL; }
			bool        isValid() const						{ return (baseFolder != NULL); }

		public:
			char        *baseFolder;
			char        *dbName;
            char        *folder_assets_bin;
            char        *folder_assets_src;
			DBHandle    db;
		};

		/*************************************
		* @brief    sBuildResult
		*/        
        struct sBuildResult
        {
        public:
            void            reset()     { uid_concrete_asset.setInvalid(); uid_virtual_asset.setInvalid(); result=eBuildResult::error; }
        
        public:
            UID             uid_concrete_asset;
            UID             uid_virtual_asset;
            eBuildResult    result;
        };         

    } //namespace asset2

} //namespace gos

#endif //_gosAsset2EnumAndDefine_h_