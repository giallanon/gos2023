#ifndef _gosAsset2Builder_h_
#define _gosAsset2Builder_h_
#include "builders/gosAsset2BuilderInterface.h"
#include "string/gosStringList.h"

namespace gos
{
	namespace asset2
	{
		/**************************************************
		* Builder
		* 
		*/
		class Builder
		{
        public:
                                //appende a <in_out> un report ben formattato con la lista delle dipendenze
            static void         get_dependencies_report (gos::UTF8String &in_out, const char *baseFolder, asset2::eFilter filter = eFilter::both);
            
                                //chiama <get_dependencies_report> e poi printf
            static void         print_dependencies_report (const char *baseFolder, asset2::eFilter filter = eFilter::both);
            
                                //chiama <get_dependencies_report> e poi salva un file di testo in /assets/src/__dependencies.txt
            static void         save_dependencies_report (const char *baseFolder, asset2::eFilter filter = eFilter::both);

		public:
						Builder (gos::GPU *gpuIN);
						~Builder();

                        template<class TBUILDER>
            bool        addBuilder ()
                        {
                            TBUILDER *builder = GOSNEW(localAllocator, TBUILDER)();
                            const u32 depth = TBUILDER::calc_depth();
                            if (priv_addBuilder(builder, depth))
                                return true;
                            GOSDELETE(localAllocator, builder);
                            return false;
                        }

			bool		rebuildAll (const char *baseFolder);
			bool		build (const char *baseFolder);

            void        debug_sanityCheck (const char *baseFolder);

        private:
            enum class eBuildStatus : u8
            {
                NEW         = 1,
                MODIFIED    = 2,
                DELETED     = 3,
                UNCHANGED   = 4
            };

        private:
            struct sResListElem
            {
            public:
                void        reset() { uid._uid=0; abspath[0]=0x00; lastTimeModified=0; status=eBuildStatus::UNCHANGED; }

            public:
                UID             uid;
                char            abspath[512];
                u64             lastTimeModified;
                eBuildStatus    status;
            };

        private:
            static constexpr u8 NUM_MAX_BUILDERS = 32;
        private:
            typedef gos::FastArray<sResListElem>            ResList;
            typedef gos::SlowHashMap<UID, gos::UTF8String>  HashedStringList;

        private:
            const char*     enumToString (eBuildStatus s) const;
            bool            priv_addBuilder (BuilderInterface *builder, u32 asset_depth);
            void            priv_printResList (const ResList &list) const;
            void            priv_printResListElem (const sResListElem &elem) const;
            bool            priv_extractAllInludePaths (const char *absFilename, gos::StringList *out) const;
            u32             priv_getDepthByAssetType (eAssetType assType) const                                         { assert (static_cast<u8>(assType) < NUM_MAX_BUILDERS); return depthByAssetTypeList[static_cast<u8>(assType)]; }
            void            priv_fromDirectiveNameToAssetClassName (const char *directiveName, char *out_asseetClassName, u32 sizeof_out) const;

		private:
            bool		priv_build (DBContext &ctx, bool bDoCreateAssetFile);
            void		priv_resource_scan_DB (DBContext &ctx, HashedStringList *out_listof_gosassetd_toRebuild, HashedStringList *out_listOfDeleteAssets) const;
            
			bool		priv_gosassetd_scan_folder (DBContext &ctx, const char *folder_path, HashedStringList *out_listof_gosassetd_toRebuild) const;
            bool        priv_gosassetd_scan_folder_parse (DBContext &ctx, const char *filename, HashedStringList *out_listof_gosassetd_toRebuild) const;
            bool        priv_gosassetd_build (DBContext &ctx, const char *absFilename, HashedStringList *out_listOfBuiltAssets);
            bool        priv_gosassetd_buildSection (DBContext &ctx, const char *absFilename, UID uid_of_iniFile, gos::IniFileSection *section, HashedStringList *out_listOfBuiltAssets);

            BuilderInterface*   priv_findBuilderByClassName (const char *assetClassName) const;

            bool        debug_sanityCheck__compareDB (DBContext &ctxSanity, const char *baseFolder);
            u32         debug_sanityCheck__count (db::RST &rst) const;
            bool        debug_sanityCheck__compare (db::RST &rst1, db::RST &rst2, u32 rowIndex) const;
            bool        debug_sanityCheck__cmp_table (DBContext &ctx_sanity, DBContext &ctx, const char *sql, const char *tableName) const;
		
        private:
			gos::Allocator	    *localAllocator;
            gos::GPU            *gpu;
            gos::Logger         *logger;
            gos::LoggerNull     loggerNull;
            BuilderInterface    *builderList[NUM_MAX_BUILDERS];
            u32                 depthByAssetTypeList[NUM_MAX_BUILDERS];

            u64                 buildTime_UTC;
            u32                 nextAnonymAssetName;

		};

	} //namespace asset2
} //namespace gos


#endif //_gosAsset2Builder_h_


