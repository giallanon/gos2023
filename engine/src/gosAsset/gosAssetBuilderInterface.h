#ifndef _gosAssetBuilderInterface_h_
#define _gosAssetBuilderInterface_h_
#include "gosAssetEnumAndDefine.h"
#include "gosIniFile.h"

namespace gos
{
    namespace asset
    {
        class Builder;  //fwd decl


        /**
         * @brief   BuilderInterface
         * 
         */
        class BuilderInterface
        {
        public:
                            BuilderInterface (eAssetType assTypeIN)                         { assType = assTypeIN; }
            virtual         ~BuilderInterface()                                             { }

            eAssetType      getAssType() const                                              { return assType; }


            virtual bool    build (Context &ctx, u64 buildTimeUTC, const asset::UID &uid_of_iniFile, const gos::IniFileSection *sec, sBuildResult *out) = 0;
            
        protected:
            bool            prot_needResolvedSubsection (Context &ctx, const gos::IniFileSection *sec, eAssetType assType, asset::UID *out_uid) const;
            bool            prot_needResource (Context &ctx, eResType resType, const char *resName, asset::UID *out_uid) const;

        private:
            eAssetType assType;
        }; //class BuilderInterface

    } //namespace asset
} //namespace gos

#endif //_gosAssetBuilderInterface_h_