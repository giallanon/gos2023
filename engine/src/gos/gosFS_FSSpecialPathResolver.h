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
            bool        addAlias (const char *alias, const u8 *realPathNoSlash);

            void	    resolve (const char *path, u8 *out, u32 sizeof_out) const           { return resolve(reinterpret_cast<const u8*>(path), out, sizeof_out); }
            void        resolve (const u8 *path, u8 *out, u32 sizeof_out) const;

        private:
            struct sAlias
            {
                u8  *alias;
                u8  *realPathNoSlash;
            };

        private:
            bool            priv_resolve (const u8 *path, u8 *out, u32 sizeof_out)  const;
            const sAlias*   priv_findAlias (const u8 *alias) const;

        private:
            gos::Allocator          *allocator;
            gos::FastArray<sAlias> listString;
        };        
    } //namespace fs
} //namespace gos


#endif //_gosFS_FSSpecialPathResolver_h_