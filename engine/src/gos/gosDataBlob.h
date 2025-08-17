#ifndef _gosDataBlob_h_
#define _gosDataBlob_h_
#include "gosEnumAndDefine.h"
#include "gosBufferWriter.h"
#include "gosLIFOFixedSize.h"


namespace gos
{

    /**
     * @brief gos::datablob
     * 
     * <blob> e' un generico blocco di memoria che puo' essere visto come una struttura dati ben formattata.
     * 
     * La classe Builder e' utilizzata per creare una DataBlobDef, ovvero un buffer che contiene tutte le informazioni circa
     * la struttura di un DataBlob. 
     * 
     * Un DataBlobDef e' buffer strutturato come segue e contiene una serie di descrittori di 'elementi'.
     * 
     *      0   u32     GOS_MAGIC__DATA_BLOB_DEF
     *      4   u16     total size of this data_blob_def
     *      6   u16     total size di un ipotetico data blob descritto da questo DataBlobDef
     *  
     *      Subito dopo questo header, c'e' un elenco di elementi, ciascuno descrivente un tipo di dati
     *      Ogni elemento inizia con un header di questo tipo:
     *          0   u8      what            Definisce che tipo di dati stiamo descrivendo e, di conseguenza,cosa aspettarsi
     *                                      dall'area 'dati' di questo elemento. E' di tipo eDataBlobElemType
     *          1   u8      nameLen         Comprensivo di 0x00 finale
     *          2   u16     next            Indirizzo assoluto del prossimo elemento, oppure 0xFFFF
     *          4   u16     absOffset       Offset assoluto di questo elemento all'iterno di un ipotetico DataBlob
     *          6   u16     size            Dimensione in byte di questo elemento all'interno di un ipotetico DataBlob
     *          8   char    *name           Un numero variabile di char ad indicare il nome dell'elemento. La stringa contiene lo 0x00 e la sua 
     *                                      lunghezza totale e' indicata da <nameLen>
     *          
     *          A seguire, ci possono essere ulteriori dati che dipendono dal tipo di dato che stiamo descrivendo:
     *             eDataBlobElemType::simpleType
     *                  0   u8  eDataFormat
     *                  
     *              eDataBlobElemType::structType
     *                  0   u8  numMembers
     * 
     *          Infine c'e' un padding di 1..3 byte in modo che l'intero Elem sia un multiplo di 4 in size
     */    
    namespace datablob
    {
        struct sElemHeader
        {
        public:
            //static constexpr u8 MAX_NAME_SIZE = 64;

        public:
            void    decodeFromBuffer (const u8 *buffer);
            
        public:
            eDataBlobElemType   elemType;
            u8      nameLen;
            u16     next;
            u16     absOffset;
            u16     paddedSize;
            u32     sizeof_thisHeader;
            const char    *elemName;
        };



        /**
        * @brief DefBuilder
        * Classe di comodo utilizzata per creare una DataBlobDef
        */
        class DefBuilder
        {
        public:
                            DefBuilder();    
            DefBuilder&     begin();

                    //aggiunge un tipo di dato semplice (non un array, non una struct). La dimensione
                    //del tipo di dati e' definita da <paddedSize>.
                    //Se <paddedSize> == u32MAX, allora la dimensione del tipo di dati e' calcolata automaticamente
            DefBuilder&     add_simpleType (const char *var_name, eDataFormat fmt, u32 paddedSize = u32MAX);

            DefBuilder&     struct_begin (const char *var_name);
            DefBuilder&     struct_add_simpleType (const char *var_name, eDataFormat fmt, u32 paddedSize = u32MAX);
            DefBuilder&     struct_end();

            DefBuilder&     array_begin (const char *var_name, u8 dimension);
            DefBuilder&     array_setDimensionSize (u8 dimensionIndex, u16 numElem);
            DefBuilder&     array_add_simpleType (const char *var_name, eDataFormat fmt, u32 paddedSize = u32MAX);
            DefBuilder&     array_end ();

            bool            end();

            bool            isValid() const                             { return bIsValid; }

                            //ritorna la dimensione inbyte dell'interno DataBlobDef
            u16             getDataBlobDefSize() const                  { return buffer.readU16At (4); }
            
                            //memcpia la DataBlobDef appena costruita in un generico buffer
            bool            memcpyDataBlobDef (void *dst, u32 sizeof_dst);

        private:
            u16             priv_elem_begin (eDataBlobElemType elemtype, const char *name);
            u16             priv_elem_end ();

        private:
            gos::LIFOFixedSize<u16, 32> stack;
            gos::BufferW_linear         buffer;
            bool                        bIsValid;
            u16                         sizeof_dataBlob;
        }; //class DefBuilder

        /**
        * @brief DefReader
        * Classe di comodo utilizzata per leggere una DataBlobDef
        */
        class DefReader
        {
        public:
                                DefReader(); 

            bool                begin (const void *dataBlobDef);
            u16                 dataBlob_getSize() const                        { return reader.readU16At (6); }
            bool                nextElem();

            eDataBlobElemType   elem_getType() const                            { return curElemHeader.elemType; }
            u16                 elem_getOffset() const                          { return curElemHeader.absOffset; }
            u16                 elem_getPaddedSize() const                      { return curElemHeader.paddedSize; }
            const char*         elem_getName() const                            { return curElemHeader.elemName; }

            eDataFormat         simpleType_getDataFmt() const;
            
            u8                  structType_getNumMembers() const;
            const char*         structType_getMemberName(u8 index) const;

        private:
            BufferR     reader;
            u32         pos_curElem;
            sElemHeader curElemHeader;


        }; //class DefReader


    } //namespace datablob
} //namespace gos

#endif //_gosDataBlob_h_