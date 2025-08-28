#ifndef _gosAssetBuilder_h_
#define _gosAssetBuilder_h_
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
                UNCHANGED   = 4
            };

        private:            
            static const char* enumToString (const eBuildStatus s);

        public:
                    Builder();
                    ~Builder();

                    template<class TBUILDER>
            bool    addBuilder ()
                    {
                        TBUILDER *builder = GOSNEW(localAllocator, TBUILDER)();
                        if (priv_addBuilder(builder))
                            return true;
                        GOSDELETE(localAllocator, builder);
                        return false;
                    }

            bool    rebuildAll (const char *baseFolder);
            bool    buildAll (const char *baseFolder);

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
            bool    priv_addBuilder (BuilderInterface *builder);
            BuilderInterface*   priv_getBuilder (eAssetType assType);

            void    priv_printResList (const ResList &list) const;
            bool    priv_fromSectionNameToAssetType (const char *name, eAssetType *out) const;
            u32     priv_fromSectionNameToAssetDeepAndType (const char *namee, eAssetType *out) const;

            u32     priv_collectResInfo (ResList &out_list);
            void    priv_collectResourcesFromDisk (ResList &out_list);
            void    priv_collectIniFileFromFDisk (ResList &out_list);

            u32     priv_explodeIniFile (ResList &list, gos::IniFile *out);
            void    priv_explodeIniFile_adjustSubsectionName (const char *in, char *out, u32 sizeof_out);
            bool    priv_explodeIniFile_ric (gos::IniFileSection *dst, IniFileSection *sec, const char *nameOfSRC);

            u32     priv_build_explodedIniFileInFolder (gos::IniFile &ini);
            u32     priv_build_iniSection (const IniFileSection *sec, BuilderInterface *builder, const char *runtimeName);


        private:
            gos::Allocator      *localAllocator;
            asset::Context      ctx;
            u64                 buildTimeUTC;
            u32                 nextTempNameIndex;
            u32                 nextTempSubsectionIndex;
            FastArray<BuilderInterface*>   builderList;

        }; //class Builder



    } //namespace asset
} //namespace gos

#endif // _gosAssetBuilder_h_
