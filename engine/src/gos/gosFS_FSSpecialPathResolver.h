#ifndef _gosFS_FSSpecialPathResolver_h_
#define _gosFS_FSSpecialPathResolver_h_
#include "gosEnumAndDefine.h"
#include "gosFastArray.h"

namespace gos
{
    namespace fs
    {
        /**********************************************
         * SpecialPathResolver
         * 
         * Dato un path:
         * 	- se il path inizia con @, allora al posto di @ viene automaticamente inserito il path alla "writable folder"
         * 		es: @/pippo.txt, diventa [pathWritable]/pippo.txt
         *  - se il path e' relativo (ovvero NON inizia con /), allora gli viene automaticamente prefisso il "path dell'app" piu' lo slash
         * 		es: pippo/pluto.txt, diventa [pathApp]/pippo/pluto
         *  - altrimenti ritorna il path inalterato
         */
        class SpecialPathResolver
        {
        public:
                        SpecialPathResolver ()      { allocator=NULL; }
                        ~SpecialPathResolver()      { unsetup(); }

            void        setup (gos::Allocator *allocator);
            void        unsetup();
            bool        addAlias (const char *alias, const char *realPathNoSlash);
            void        removeAlias (const char *alias);
            void        resolve (const char *path, char *out, u32 sizeof_out) const;

        private:
            struct sAlias
            {
                char  *alias;
                char  *realPathNoSlash;
            };

        private:
            bool            priv_resolve (const char *path, char *out, u32 sizeof_out)  const;
            const sAlias*   priv_findAlias (const char *alias) const;
            u32             priv_findAliasIndex (const char *alias) const;

        private:
            gos::Allocator          *allocator;
            gos::FastArray<sAlias> listString;
        };        
    } //namespace fs
} //namespace gos


#endif //_gosFS_FSSpecialPathResolver_h_
