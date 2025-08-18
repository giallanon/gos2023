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
     *      Subito dopo questo header, c'e' un elenco di elementi, ciascuno descrivente un tipo di dati.
     *      Ogni elemento inizia con un header di questo tipo:
     *          0   u8      eDataBlobElemType   Definisce che tipo di dati stiamo descrivendo e, di conseguenza,cosa aspettarsi
     *                                          dall'area 'dati' di questo elemento. E' di tipo eDataBlobElemType
     *          1   u8      nameLen             Comprensivo di 0x00 finale
     *          2   u16     next                Indirizzo assoluto del prossimo elemento, oppure 0xFFFF
     *          4   u16     absOffset           Offset assoluto di questo elemento all'iterno di un ipotetico DataBlob
     *          6   u16     size                Dimensione in byte di questo elemento all'interno di un ipotetico DataBlob
     *          8   char    *name               Un numero variabile di char ad indicare il nome dell'elemento. La stringa contiene lo 0x00 e la sua 
     *                                          lunghezza totale e' indicata da <nameLen>
     *          
     *          A seguire, ci possono essere ulteriori dati che dipendono dal tipo di dato che stiamo descrivendo:
     *             eDataBlobElemType::simpleType
     *                  0   u8  eDataFormat
     *                  1   padding di 1..7 byte in modo che l'intero Elem sia un multiplo di 8 in size
     *   
     *              eDataBlobElemType::structType
     *                  0   u16  pos of 1st child
     *                  2   u16  end pos of last child
     *                  4   u16  numMembers
     *                  6   padding di 1..7 byte in modo che l'intero Elem sia un multiplo di 8 in size
     * 
     *              eDataBlobElemType::arrayType
     *                  0   u16     pos of 1st child
     *                  2   u16     end pos of last child
     *                  4   u8      dimension   (1=array 1D, 2=array 2D, 3=array 3D...)
     *                  5   u8      size of one elem
     *                  6   u16     numElem per la dimensione 1
     *                  ...
     *                  ..  u16     numElem per la dimensione N
     *                  ..  padding di 1..7 byte in modo che l'intero Elem sia un multiplo di 8 in size
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


        bool    blobDef_isValidMagic (const void *dataBlodDef);
        u16     blobDef_getTotalSize (const void *dataBlodDef);
        u16     blobDef_getSizeOfDataBlob (const void *dataBlodDef);
        void    print_info (const char *name, const void *dataBlobDef);

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
                            //add_simpleType..
                            //...
                            //add_simpleType..
            DefBuilder&     struct_end();

            DefBuilder&     array_begin1D (const char *var_name, u16 numElem1);
            DefBuilder&     array_begin2D (const char *var_name, u16 numElem1, u16 numElem2);
            DefBuilder&     array_begin3D (const char *var_name, u16 numElem1, u16 numElem2, u16 numElem3);
                            //add_simpleType..
                            // oppure
                            //struct_begin
                            //add_simpleType..
                            //...
                            //add_simpleType..
                            //struct_end();
            DefBuilder&     array_end ();

            bool            end();

            bool            isValid() const                             { return bIsValid; }

                            //ritorna la dimensione inbyte dell'interno DataBlobDef
            u16             getDataBlobDefSize() const                  { return buffer.readU16At (4); }
            
                            //memcpia la DataBlobDef appena costruita in un generico buffer
            bool            memcpyDataBlobDef (void *dst, u32 sizeof_dst) const;

            u8*             allocDataBlobDef (gos::Allocator *allocator) const;
                            //alloca un buffer della necessaria dimensione e ci memcpia la DataBlobDef

        private:
            u16             priv_elem_begin (eDataBlobElemType elemtype, const char *name);
            u16             priv_elem_end ();
            void            priv_add_pad();
            u16             priv_array_begin_start (const char *var_name, u8 numDimension);
            void            priv_array_begin_end(u16 pos_posOfFirstChild);

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
            class Elem
            {
            public:
                                    Elem()                                      { }
                                    ~Elem()                                     { }

                eDataBlobElemType   getType() const                             { return header.elemType; }
                u16                 getOffset() const                           { return header.absOffset; }
                u16                 getPaddedSize() const                       { return header.paddedSize; }
                const char*         getName() const                             { return header.elemName; }

                eDataFormat         simpleType_getDataFmt() const;
                
                u8                  structType_getNumMembers() const;
                const char*         structType_getMemberName(u8 index) const;
                bool                structType_getFirstMember (Elem *out) const;

                u8                  arrayType_getNumDimension() const;
                u16                 arrayType_getNumElem (u8 index) const;
                u8                  arrayType_getSizeOfOneElem() const;
                bool                arrayType_getFirstMember (Elem *out) const;

                bool                next();

            private:
                void                priv_setup (const BufferR *reader, u16 startingPos, u16 endingPos);

            private:
                const BufferR   *reader;
                sElemHeader     header;
                u16             pos_curElem;
                u16             endingPos;

            friend DefReader;
            };

        public:
                                DefReader()                                     { }
                                ~DefReader()                                    { }

            bool                begin (const void *dataBlobDef, Elem *out);
            u16                 dataBlob_getSize() const                        { return reader.readU16At (6); }

        private:
            BufferR     reader;
        }; //class DefReader


    } //namespace datablob
} //namespace gos

#endif //_gosDataBlob_h_