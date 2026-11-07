#ifndef _gosAsset2BuilderInterface_h_
#define _gosAsset2BuilderInterface_h_
#include "../gosAsset2EnumAndDefine.h"
#include "../gosAsset2.h"
#include "../gosGPU/gosGPU.h"
#include "gosIniFile.h"
#include "string/gosStringList.h"

namespace gos
{
    namespace asset2
    {
        class Builder; //FWD
		class BuilderInterface; //FWD
		class BuilderList; //FWD


        /*******************************
         * @brief ResourceDep
         * 			Se un asset necessita di una risorsa, allora nei suoi "param"
		 * 			deve indicare un parametro di tipo ResourceDep
         */
		struct ResourceDep
		{
			UID	resourceUID;
			u64	lastTimeMod;
		};

        
		/*******************************
         * @brief BuilderInterface
         * 
         */
        class BuilderInterface
        {
        public:
                            BuilderInterface (eAssetType assetTypeIN)                                   { assetType=assetTypeIN; logger=NULL; }
            virtual         ~BuilderInterface()                                                         { }

            eAssetType      getSignatureType() const                                                 	{ return assetType; }

            virtual void    initOnce (gos::GPU *gpu, BuilderList *builderList)							{ }
            virtual void    deinitOnce()                                                                { }

            void            setLogger (gos::Logger *l)                                                  { logger= l; }


			virtual	bool 	build_begin (DBContext &ctx, const UniqueUIDList *listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec) = 0;
			virtual bool 	build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result) = 0;
			virtual void 	build_end() = 0;

		protected:
			static constexpr u8	NUM_MAX_NESTED_RESOURCES = 64;

        protected:
            bool            prot_isOneOfThis (const char *paramName, ...) const;
            bool            prot_needResource (DBContext &ctx, const UniqueUIDList *listof_UID_of_known_ini_file, eResType resType, const char *absFilenameIN, ResourceDep *out);
            bool            prot_needResolvedSubsection (DBContext &ctx, const gos::IniFileSection *sec, eAssetType assType, UID *out__assetUID) const;
            bool            prot_setupAsset (DBContext &ctx, const void *params, u32 sizeof_params, UID uid_of_iniFile, const gos::IniFileSection *sec, sBuildResult *out_result) const;
			bool 			prot_setupAsset_ex (DBContext &ctx, eAssetType assetType, const void *params, u32 sizeof_params, const char *rtname, UID uid_of_iniFile, u32 asset__declared_on_lineNum, sBuildResult *out_result) const;
			bool 			prot_add_asset_to_asset_dependency (DBContext &ctx, sBuildResult *padre, UID uid_asset_figlio, UID *out__uid_signature_figlio = NULL);
			bool 			prot_add_asset_to_resouce_dependency (DBContext &ctx, sBuildResult *padre, UID uid_resource_figlio);

        protected:
            gos::Logger     *logger;
			ResourceDep		list_of_nested_resources[NUM_MAX_NESTED_RESOURCES];
			u32				num_nested_resources;

        private:
            bool            priv_extractAllInludePaths (DBContext &ctx, const UniqueUIDList *listof_UID_of_known_ini_file, const char *absFilenameIN, gos::StringList *out) const;
			bool            priv_do_needResource (DBContext &ctx, const UniqueUIDList *listof_UID_of_known_ini_file, eResType resType, const char *absFilenameIN, ResourceDep *out);

        private:
            eAssetType      assetType;    

        };


		/*******************************
         * @brief BuilderList
         * 			classe di comodo, contiene un elenco di BuilderInterface
         */
		class BuilderList
		{
		public:
				BuilderList() 											{ gpu=NULL; localAllocator=NULL; memset(builderList, 0, sizeof(builderList)); }
				~BuilderList()											{ unsetup(); }

			void	setup (gos::Allocator *alloctor, gos::GPU *gpuIN)	{ gpu=gpuIN; localAllocator=alloctor; }
			void	unsetup()
			{
				for (u32 i = 0; i < NUM_MAX_BUILDERS; i++)
				{
					if (NULL != builderList[i])
					{
						builderList[i]->deinitOnce();
						GOSDELETE(localAllocator, builderList[i]);
					}
				}					
			}			


                    template<class TBUILDER>
            bool	add_builder ()
					{
						TBUILDER *builder = GOSNEW(localAllocator, TBUILDER)();
						if (priv_addBuilder(builder))
							return true;
						GOSDELETE(localAllocator, builder);
						return false;
					}				

			BuilderInterface*	findBuilderByClassName (const char *assetClassName) const
			{
				for (u32 i = 0; i < NUM_MAX_BUILDERS; i++)
				{
					if (NULL == builderList[i])
						continue;

					if (0 == strcmp(asset2::enumToString(builderList[i]->getSignatureType()), assetClassName))
						return builderList[i];
				}
				return NULL;
			}
			
        private:
            static constexpr u8 NUM_MAX_BUILDERS = 32;

		private:
			bool priv_addBuilder(BuilderInterface *builder)
			{
				assert(NULL != builder);

				const u32 index = static_cast<u8>(builder->getSignatureType());
				assert(index < NUM_MAX_BUILDERS);

				if (NULL == builderList[index])
				{
					builderList[index] = builder;
					builder->initOnce(gpu, this);
					return true;
				}

				//logger->err("BuilderList::priv_addBuilder() => a builder for res %s already exists\n", asset2::enumToString(builder->getSignatureType()));
				DBGBREAK;
				return false;
			}
			

		private:
			gos::Allocator		*localAllocator;
			gos::GPU 			*gpu;
			BuilderInterface	*builderList[NUM_MAX_BUILDERS];
		};		

    } //namespace asset2
} //namespace gos

#endif //_gosAsset2BuilderInterface_h_
