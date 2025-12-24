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

            virtual bool    build (DBContext &ctx, u64 buildTime_UTC, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out) = 0;

        protected:
            void            prot_makeABSPath (const char *absFilename, const char *path, char *out, u32 sizeof_out) const;
            bool            prot_isOneOfThis (const char *paramName, ...) const;
            bool            prot_needResource (DBContext &ctx, eResType resType, const char *absFilenameIN, UID *out_uid) const;
            bool            prot_needResolvedSubsection (DBContext &ctx, const gos::IniFileSection *sec, eAssetType assType, UID *out_uid) const;

        protected:
            gos::Logger     *logger;

        private:
            bool priv_extractAllInludePaths (const char *absFilenameIN, gos::StringList *out) const;

        private:
            eAssetType      assetType;    

        };

    } //namespace asset2
} //namespace gos

#endif //_gosAsset2BuilderInterface_h_
