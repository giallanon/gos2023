#ifndef _gosStringIncludeDetector_h_
#define _gosStringIncludeDetector_h_
#include "../gosString.h"
#include "../gosFastArray.h"


namespace gos
{
    namespace string
    {
        /***************************************************************************
         * 
         * @brief 	IncludeDetector
         *          Parsa un file di testo alla ricerca delle direttive #include "<file-name>" classiche del c/c++
         *          
         */
        class IncludeDetector
        {
        public:
                    IncludeDetector();
                    ~IncludeDetector()                                                          { }

                    //prser() ritorna il num di #define trovate
            u32		parse (const u8 *bufferSRC, u32 sizeof_buffer)                              { return parse (reinterpret_cast<const char*>(bufferSRC), sizeof_buffer); }
            u32		parse (const char *bufferSRC, u32 sizeof_buffer)                            { string::utf8::Iter src; src.setup (bufferSRC, 0, sizeof_buffer); return parse (src); }
            u32		parse (string::utf8::Iter &src);

            u32 	getNumResults() const												        { return list.getNElem(); }

                    /**
                     * @param   out_startAtByte indica l'offset all'interno del buffer analizzato al quale inizia <file-name>
                     * @param   out_len indica la lunghezza di <file-name> (non includendo lo 0x00)
                     */
            bool	getResultByIndex (u32 index, u32 *out_startAtByte, u32 *out_len) const;
            bool    getResultAsString (const u8 *bufferSRC, u32 index, char *out, u32 sizeof_out) const;
            bool    getResultAsString (const char *bufferSRC, u32 index, char *out, u32 sizeof_out) const;

        private:
            struct sElem
            {
                u32	startAt;
                u32 len;
            };

        private:
            gos::FastArray<sElem>	list;
        };

    } //namespace string
} //namespace gos

#endif //_gosStringIncludeDetector_h_            