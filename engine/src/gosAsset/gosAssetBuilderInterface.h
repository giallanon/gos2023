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


            virtual bool    build (Context &ctx, u64 buildTimeUTC, const gos::IniFileSection *sec, sBuildResult *out) = 0;
            
        private:
            eAssetType assType;
        }; //class BuilderInterface

    } //namespace asset
} //namespace gos

#endif //_gosAssetBuilderInterface_h_