#ifndef _gosThreadMsgQ_h_
#define _gosThreadMsgQ_h_
#include "gos.h"
#include "gosHandle.h"


struct HThreadMsgR 
{
    GOSThreadMsgHandle    hRead;

    bool    operator== (const HThreadMsgR &b) const                        { return (hRead==b.hRead); }
    bool    operator!= (const HThreadMsgR &b) const                        { return (hRead!=b.hRead); }
};

struct HThreadMsgW
{
    GOSThreadMsgHandle    hWrite;

    bool    operator== (const HThreadMsgW &b) const                        { return (hWrite==b.hWrite); }
    bool    operator!= (const HThreadMsgW &b) const                        { return (hWrite!=b.hWrite); }
};


namespace gos
{
    namespace thread
    {
        struct sMsg
        {
            u16         what;
            u32         paramU32;
            void        *buffer;
            u32         bufferSize;
        };


        /************************************************************
         * uso interno
         */
        bool            internal_init();     //uso interno (usate da gos::init())
        void            internal_deinit();   //uso interno (usate da gos::deinit())




        /************************************************************
         * comunicazione tra thread
         *
         * createMsgQ() crea una FIFO thread safe e ritorna 2 handle, uno da usare per scrivere sulla FIFO (HandleW), l'altro per leggere (HandleR).
         * Un thread(1) riceverà HandleR e sarà solo in grado di leggere da quella FIFO.
         * Gli altri thread, eventualmente, riceverannno l'HandleW e saranno solo in grado di scrivere su quella FIFO, notificando cosi' il thread(1).
         *
         * Ogni volta che qualcuno scrive sulla FIFO (vedi push..), un OSEvent dedicato alla FIFO viene fired(). Il thread(1), può quindi stare in attesa
         * sull'OSEvent in modo da venire notificato quando qualcuno gli ha mandato un msg (ie: non è necessario pollare sulla pop() all'infinito, si può stare in attesa che l'evento
         * venga fired).
         *
         * Ogni volta che si pop() un messaggio, è necessario poi chiamare deleteMsg() per liberare l'eventuale memoria da esso allocata.
         *
         * pushMsgAsBuffer() fa una copia locale del contenuto puntato da *src. L'allocazione ed il free del buffer-copia sono gestite internamente, non c'è da preoccuparsene.
         * La chiamata a deleteMsg() infatti, libera l'eventuale memoria allocata per il buffer.
         *
         * deleteMsgQ() fa il free della FIFO e di tutti gli eventuali msg ancora in coda (fa il free anche dell'OSEvent). ATTENZIONE a quando fai il deleteMSgQ. E' bene accertarsi
         * che non ci siano in giro thread con handle che "puntano" alla coda che vuoi deletare.
         */
        bool            createMsgQ (HThreadMsgR *out_handleR, HThreadMsgW *out_handleW);
        void            deleteMsgQ (HThreadMsgR &handleR, HThreadMsgW &handleW);
        
        u32             calcSizeNeededToSerializeMsg (const sMsg &msg);
        u32             serializeMsg (const sMsg &msg, u8 *out_buffer, u32 sizeof_out_buffer);
        u32             deserializMsg (const u8 *buffer, u16 *out_what, u32 *out_paramU32, u32 *out_bufferSize, const u8 **out_bufferPt);

                        //read
        bool            getMsgQEvent (const HThreadMsgR &h, gos::Event *out_hEvent);
        bool            popMsg (const HThreadMsgR &h, sMsg *out_msg);
        void            deleteMsg (const sMsg &msg);

                        //write
        void            pushMsg (const HThreadMsgW &h, u16 what, u32 paramU32, const void *src, u32 sizeInBytes);
        inline void     pushMsg (const HThreadMsgW &h, u16 what, u32 paramU32)                                           { pushMsg(h, what, paramU32, NULL, 0); }
        inline void     pushMsg (const HThreadMsgW &h, u16 what, const void *src, u32 sizeInBytes)                       { pushMsg(h, what, 0, src, sizeInBytes); }
        void            pushMsg2Buffer (const HThreadMsgW &h, u16 what, u32 paramU32, const void *src1, u32 sizeInBytes1, const void *src2, u32 sizeInBytes2);
                            //appende src2 subito dopo src1. Il msg.bufferSize diventa size1+size2.
                            //E' solo una fn di comodo

    } //namespace thread

} // namespace gos



#endif // _gosThreadMsgQ_h_
