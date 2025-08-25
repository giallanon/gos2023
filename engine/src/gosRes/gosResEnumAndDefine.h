#ifndef _gosResEnumAndDefine_h_
#define _gosResEnumAndDefine_h_
#include "gosEnumAndDefine.h"
#include "gosIniFile.h"
#include "gosDB.h"

namespace gos
{
    namespace res
    {
        class Builder; //fwd decl

        struct sBuilderSession
        {
            const char *baseFolder;
            DBHandle    db;
            u64         timestamp;
        };

        /**
         * @brief   IResBuilder
         *          Interfaccia per un generico builder di risorsa.
         *          Ogni risorsa deve implementare il suo builder
         */
        class IResBuilder
        {
        public:
                            IResBuilder (eResType resTypeIN)                        { resType = resTypeIN; }
            virtual         ~IResBuilder()                                          { }

            virtual bool    build (sBuilderSession &session, const IniFileSection *sec, u64 lastTimeIniSectionWasUpdate) = 0;

            eResType        getResType() const                                      { return resType; }

        private:
            eResType        resType;

        }; //class IResBuilder
    };

} //namespace gos

#endif //_gosResEnumAndDefine_h_