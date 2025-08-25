#ifndef _gosResBuilder_h_
#define _gosResBuilder_h_
#include "gosResEnumAndDefine.h"

namespace gos
{
    namespace res
    {
        /**
         * @brief   Builder
         *          Hub centrale per il build di tutte le risorse.
         *          Contiene una lista di ResBuilder grazie ai quali puo'
         *          buildare tutti i tipo di risorsa conosciuti
         */
        class Builder
        {
        public:
                    Builder();
                    ~Builder()                                          { priv_free(); }

                    template<class TBUILDER>
            bool    addResBuilder ()
                    {
                        TBUILDER *b = GOSNEW(localAllocator, TBUILDER)();
                        if (priv_addResBuilder(b))
                            return true;
                        GOSDELETE(localAllocator, b);
                        return false;
                    }

            bool    open (const char *baseFolder);

            void    rebuildAll();
            void    buildAll ();

        private:
            static const u8 DB__VER = 1;

        private:
            void    priv_free();
            bool    priv_openDB();
            bool    priv_createEmptyDB (const char *dbFile);
            void    priv_updateDBToCurrentDBVer();
            bool    priv_addResBuilder (IResBuilder *builder);
            void    priv_buildFolder (sBuilderSession &session, const char *folder);
            void    priv_buildFile (sBuilderSession &session, const char *fileFullPathAndName);
            void    priv_do_buildFile (sBuilderSession &session, const char *fileFullPathAndName);
            void    priv_newSession (sBuilderSession *out) const;

        private:
            gos::Allocator      *localAllocator;
            char                *baseFolder;
            gos::FastArray<IResBuilder*>  builderList;
            DBHandle            db;
        };

    } //namespace res
} //namespace gos


#endif //_gosResBuilder_h_