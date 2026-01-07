#ifndef _gosAsset2Builder_h_
#define _gosAsset2Builder_h_
#include "builders/gosAsset2BuilderInterface.h"
#include "string/gosStringList.h"
#include "string/gosUniqueStringList.h"

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
            static void         get_dependencies_report (gos::UTF8String &in_out, const char *baseFolder, const char *dbName = NULL);
            
                                //chiama <get_dependencies_report> e poi printf
            static void         print_dependencies_report (const char *baseFolder, const char *dbName = NULL);
            
                                //chiama <get_dependencies_report> e poi salva un file di testo in /assets/src/__dependencies.txt
            static void         save_dependencies_report (const char *baseFolder, const char *dbName = NULL);

            static void         save_asset_manifest (const char *baseFolder, const char *dbName = NULL);

		public:
						Builder (gos::GPU *gpuIN);
						~Builder();

                        template<class TBUILDER>
            bool        addBuilder ()
                        {
                            TBUILDER *builder = GOSNEW(localAllocator, TBUILDER)(this);
                            if (priv_addBuilder(builder))
                                return true;
                            GOSDELETE(localAllocator, builder);
                            return false;
                        }

			bool		rebuildAll (const char *baseFolder, bool bVerbose);
			bool		build (const char *baseFolder, bool bVerbose);

            bool        debug_sanityCheck (const char *baseFolder);


        public:
            bool        internal__makeABSPathFromFilename (const char *origin_absFilename, const char *rel_or_abs_path, char *out, u32 sizeof_out) const;

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

            struct sAlias
            {
                char *alias;
                char *path;
            };

        private:
            static constexpr u8 NUM_MAX_BUILDERS = 32;
        private:
            typedef gos::FastArray<sResListElem>            ResList;
            typedef gos::SlowHashMap<UID, gos::UTF8String>  HashedStringList;

        private:
            const char*     enumToString (eBuildStatus s) const;
            bool            priv_addBuilder (BuilderInterface *builder);
            void            priv_printResList (const ResList &list) const;
            void            priv_printResListElem (const sResListElem &elem) const;
            void            priv_fromDirectiveNameToAssetClassName (const char *directiveName, char *out_asseetClassName, u32 sizeof_out) const;

		private:
            bool		priv_build (DBContext &ctx, bool bDoCreateAssetFile);
            bool		priv_resource_scan_DB (DBContext &ctx, HashedStringList *out_listof_gosassetd_toRebuild, UniqueUIDList *out_listof_deleted_gosassetd, UniqueUIDList *out_listOfPossibileAssetsToBeDeleted) const;
            
			bool		priv_gosassetd_scan_folder (DBContext &ctx, const char *folder_path, HashedStringList *out_listof_gosassetd_toRebuild) const;
            bool        priv_gosassetd_scan_folder_parse (DBContext &ctx, const char *filename, HashedStringList *out_listof_gosassetd_toRebuild) const;
            bool        priv_gosassetd_build (DBContext &ctx, bool bDoCreateAssetFile, const char *absFilename, UniqueUIDList *out_listOfBuiltAssets);
            bool        priv_gosassetd_build_parseIncludeSection (DBContext &ctx, bool bDoCreateAssetFile, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sub, UniqueStringList &in_out_listof_knownRTname, UniqueUIDList *out_listOfBuiltAssets);
            bool        priv_gosassetd_build_parseAliasSection (const char *absFilename, const gos::IniFileSection *sub);
            bool        priv_gosassetd_buildSection (DBContext &ctx, bool bDoCreateAssetFile, u32 &in_out_nextAnonymAssetName, UniqueStringList &in_out_listof_knownRTname, const char *absFilename, UID uid_of_iniFile, gos::IniFileSection *section, UniqueUIDList *out_listOfBuiltAssets);

            BuilderInterface*   priv_findBuilderByClassName (const char *assetClassName) const;
            
            u32         priv_alias_findIndexByName (const char *alias) const;
            void        priv_alias_deleteAll();

            bool        debug_sanityCheck__compareDB (DBContext &ctxSanity, const char *baseFolder);
            u32         debug_sanityCheck__count (db::RST &rst) const;
            bool        debug_sanityCheck__compare (db::RST &rst1, db::RST &rst2, u32 rowIndex) const;
            bool        debug_sanityCheck__cmp_table (DBContext &ctx_sanity, DBContext &ctx, const char *sql, const char *tableName) const;
		
        private:
			gos::Allocator	    *localAllocator;
            gos::GPU            *gpu;
            gos::Logger         *logger;
            gos::LoggerNull     loggerNull;
            gos::LoggerStdout   loggerStdout;
            BuilderInterface    *builderList[NUM_MAX_BUILDERS];
            gos::FastArray<sAlias>  aliasList;

            u64                 buildTime_UTC;

		};

	} //namespace asset2
} //namespace gos


#endif //_gosAsset2Builder_h_


