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
        /*******************************
         * @brief BuilderInterface
         * 
         */
        class BuilderInterface
        {
        public:
                            BuilderInterface (eAssetType assetTypeIN)           { assetType=assetTypeIN; logger=NULL; }
            virtual         ~BuilderInterface()                                 { }

            eAssetType      getAssetType() const                                { return assetType; }

            virtual void    initOnce (gos::GPU *gpu)                            { }
            virtual void    deinitOnce()                                        { }

            void            setLogger (gos::Logger *l)                          { logger= l; }

            virtual bool    build (DBContext &ctx, u64 buildTime_UTC, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out_result) = 0;

        protected:
            bool            prot_isOneOfThis (const char *paramName, ...) const;
            bool            prot_needResource (DBContext &ctx, eResType resType, const char *absFilenameIN, UID *out_uid) const;
            bool            prot_needResolvedSubsection (DBContext &ctx, const gos::IniFileSection *sec, eAssetType assType, UID *out__virtual_uid) const;
            bool            prot_seuptVirtualAsset (DBContext &ctx, const void *params, u32 sizeof_params, UID uid_of_iniFile, const gos::IniFileSection *sec, sBuildResult *out_result) const;

        protected:
            gos::Logger     *logger;

        private:
            bool            priv_extractAllInludePaths (const char *absFilenameIN, gos::StringList *out) const;

        private:
            eAssetType      assetType;    

        };

    } //namespace asset2
} //namespace gos

#endif //_gosAsset2BuilderInterface_h_
