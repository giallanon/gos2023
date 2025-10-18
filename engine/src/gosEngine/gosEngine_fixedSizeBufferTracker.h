#ifndef _gosEngine_fixedSizeBufferTracker_h_
#define _gosEngine_fixedSizeBufferTracker_h_
#include "gosEngineEnumAndDefine.h"
#include "../gos/gosBit.h"


namespace gos
{
    namespace engine
    {
        /************************************
         * @brief   FixedSizeBufferTracker
         *          Gestice un ipotetico buffer di <numMaxObject> elementi.
         *          Per come e' costruito <Handle>, <numMaxObject> puo' valere al max 0xFFFF.
         * 
         *          Per richiedere uno slot nel buffer, chiamare bind() la quale eventualmente ritorna un valido Handle dentro al quale
         *          c'e' l'<index> da usare all'interno dell'ipotetico buffer.
         *          Questo handle, nel futuro, puo' diventare invalido, per esempio perche' qualcuno chiama unbind() o a seguito della pulizia del buffer.
         *          A tale proposito, e' sempre necessario chiamare isBound() per assicurarsi che l'handle punti ancora alla risorsa che era stata riservata.
         *          Se cosi' non fosse, e' necessario nuovamente chiamare bind() per ottenere un nuovo handle
         *          
         * 
         */
        class FixedSizeBufferTracker
        {
        public:
                    FixedSizeBufferTracker();
                    ~FixedSizeBufferTracker()                                                                                     { unsetup(); }

            void    setup (gos::Allocator *allocator, u32 numMaxObject);
            void    unsetup ();
            
            bool    bind (ResHandle *out);
            void    unbind (ResHandle handle);
            bool    isBound (ResHandle handle) const;

        private:
            gos::Allocator  *allocator;
            u16             *valid_if;
            gos::Bitfield   is_busy;
            u32             numMaxObject;
            u32             numObject;

        };
    } //namespace engine
} //namespace gos



#endif //_gosEngine_fixedSizeBufferTracker_h_