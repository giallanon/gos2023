#ifndef _gosErr_h_
#define _gosErr_h_
#include "../gosEnumAndDefine.h"
#include "../gosBufferLinear.h"

namespace gos
{
    /**
     * @brief MultiThreadErrHandler
     * 
     * Gestisce un "ThreadErr" per ogni thread.
     * Le singole istanze di "ThreadErr" non vengono allocate fino a quando effettivamente un thread non chiama la "vadd"; e' solo
     * in quel momento che si creare una nuova istanza di "ThreadErr" (a meno che non ne esista gia' una associata al threadID in questione).
     * 
     * Questa classe e' una utility class usata dalle funzioni del namespace gos::err
     */
    class MultiThreadErrHandler
    {
    public:
                        MultiThreadErrHandler();
                        ~MultiThreadErrHandler();

		void 			clear (u32 threadID);
		void 			add (u32 threadID, const char *format, ...)                 { va_list argptr; va_start (argptr, format); vadd (threadID, format, argptr); va_end (argptr); }
        void            vadd (u32 threadID, const char *format, va_list argptr);
		u32				getErrCount (u32 threadID);
		const char*		getErrByIndex (u32 threadID, u32 i);

        void            deleteThisHandlerIfExists (u32 threadID);

    private:
        static const u8 NUM_MAX_HANDLER = 32;

    private:
        class ThreadErr
        {
        public:
                            ThreadErr();
                            ~ThreadErr()                                    { buffer.unsetup(); }

            void            reset()                                         { errCount = 0; offset = 0; }
            void            vadd (const char *format, va_list argptr);

            u32             getCount() const                                { return errCount; }
            const char*     getErrByIndex (u32 i) const;

        private:
            

        private:
            BufferLinear    buffer;
            u32             offset;
            u32             errCount;
            u8              baseMemBlock[256];
        };

        struct sRecord
        {
            u32 threadID;
            ThreadErr *err;
        };

    private:
        ThreadErr*      exists (u32 threadID) const;
        ThreadErr*      create (u32 threadID);

    private:
        char            NULLSTR[8];
        gos::Mutex      mutex;
        sRecord         handlerList[NUM_MAX_HANDLER];
        
    };
} //namespace gos

#endif //_gosErr_h_