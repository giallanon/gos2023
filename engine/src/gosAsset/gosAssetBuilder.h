#ifndef _gosAssetBuilder_h_
#define _gosAssetBuilder_h_
#include "logger/gosLoggerNull.h"
#include "gosAsset.h"
#include "gosAssetBuilderInterface.h"

namespace gos
{
    namespace asset
    {
        /**
         * @brief   Builder
         * 
         */
        class Builder
        {
        private:
            enum class eBuildStatus : u8
            {
                DONT_KNOW   = 0,
                NEW         = 1,
                MODIFIED    = 2,
                DELETED     = 3,
                UNCHANGED   = 4,
                REQUIRED    = 5
            };

        private:            
            static const char*  enumToString (const eBuildStatus s);

        public:
            static void         print_dependencies (const char *baseFolder, asset::eFilter filter = eFilter::both);

        public:
                    Builder();
                    ~Builder();

                    template<class TBUILDER>
            bool    addBuilder ()
                    {
                        TBUILDER *builder = GOSNEW(localAllocator, TBUILDER)();
                        const u32 depth = TBUILDER::calc_depth();
                        if (priv_addBuilder(builder, depth))
                            return true;
                        GOSDELETE(localAllocator, builder);
                        return false;
                    }

            bool    rebuildAll (const char *baseFolder, bool bVerbose);
            bool    buildAll (const char *baseFolder, bool bVerbose);


            void    debug_sanityCheck (const char *baseFolder);

        private:
            static constexpr u8 NUM_MAX_ASSET_BUILDER = 32;
            static constexpr char DB_NAME[] = { "assets.sqlite3" };

        private:
            struct sResListElem
            {
            public:
                void        reset() { uid._uid=0; name[0]=0x00; lastTimeModified=0; resType=eResType::__FINISHED; status=eBuildStatus::DONT_KNOW; }
            public:
                asset::UID  uid;
                char        name[128];
                u64         lastTimeModified;
                eResType    resType;
                eBuildStatus  status;
            };

            typedef gos::FastArray<sResListElem>    ResList;

        private:
            bool    priv_addBuilder (BuilderInterface *builder, u32 asset_depth);
            BuilderInterface*   priv_getBuilder (eAssetType assType);

            u32     priv_getDepthByAssetType (eAssetType assType) const                                         { assert (static_cast<u8>(assType) < NUM_MAX_ASSET_BUILDER); return depthByAssetType[static_cast<u8>(assType)]; }
            void    priv_closeAllContext();


            u32     priv_do_build (Context &ctx);

            void    priv_printResList (const ResList &list) const;
            bool    priv_fromSectionNameToAssetType (const char *name, eAssetType *out) const;
            u32     priv_fromSectionNameToAssetDepthAndType (const char *name, eAssetType *out) const;

            u32     priv_collectResInfo (ResList &out_list);
            void    priv_collectResourcesFromDisk (ResList &out_list);
            void    priv_collectIniFileFromDisk (ResList &out_list);

            //bool    priv_explode_NEW_or_MODIFIED_res (ResList *in_out_list);
            void    priv_explodeIniFile_adjustSubsectionName (const char *in, char *out, u32 sizeof_out);
            bool    priv_explodeScript_ric (gos::IniFileSection *dst, IniFileSection *sec, const char *nameOfSRC, const asset::UID &uid_of_iniFile);

            u32     priv_build_explodedIniFileInFolder (gos::IniFile &ini);
            u32     priv_build_iniSection (const IniFileSection *sec, const asset::UID &uid_of_iniFile, const char *sourceFileInfo, BuilderInterface *builder, const char *runtimeName);


            bool    debug_sanityCheck_run (const char *baseFolder);
            u32     debug_sanityCheck__count (db::RST &rst) const;
            bool    debug_sanityCheck__compare (db::RST &rst1, db::RST &rst2, u32 rowIndex) const;
            bool    debug_sanityCheck__cmp_table (const char *sql, const char *tableName);

        private:
            gos::Allocator      *localAllocator;
            gos::Logger         *logger;
            gos::LoggerNull     loggerNull;
            
            asset::Context      ctx;
            asset::Context      ctx_backup;
            asset::Context      ctx_sanity;
            u64                 buildTimeUTC;
            u32                 nextTempNameIndex;
            u32                 nextTempSubsectionIndex;
            u32                 *depthByAssetType;
            BuilderInterface    **builderList;

        }; //class Builder



    } //namespace asset
} //namespace gos

#endif // _gosAssetBuilder_h_
