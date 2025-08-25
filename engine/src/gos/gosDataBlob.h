#ifndef _gosDataBlob_h_
#define _gosDataBlob_h_
#include "gosEnumAndDefine.h"
#include "gosBufferWriter.h"
#include "gosLIFOFixedSize.h"
#include "gosBit.h"
#include "string/gosUTF8String.h"

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
     *          8   u32     userDefined         Un 32bit per uso generico
     *          12  char    *name               Un numero variabile di char ad indicare il nome dell'elemento. La stringa contiene lo 0x00 e la sua 
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
     *                              Il bit + significativo vale 1 se l'array e' un array semplice
     *                  5   u8      stride (AKA size of one elem)
     *                  6   u16     numElem per la dimensione 1
     *                  ...
     *                  ..  u16     numElem per la dimensione N
     *                  ..  padding di 1..7 byte in modo che l'intero Elem sia un multiplo di 8 in size
     */    
    namespace datablob
    {
        class DefReader; //fwd decl

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
            u32     userDefined;
            u32     sizeof_thisHeader;
            const char    *elemName;
        };

        /**
        * @brief DefElem
        * Ottenuto da DefReader durante la scansione della struttura dati
        */
        class DefElem
        {
        public:
                                DefElem()                                      { }
                                ~DefElem()                                     { }

            eDataBlobElemType   getType() const                             { return header.elemType; }
            u16                 getOffset() const                           { return header.absOffset; }
            u16                 getPaddedSize() const                       { return header.paddedSize; }
            const char*         getName() const                             { return header.elemName; }
            eDataFormat         getDataFmt() const;
            u32                 getUserData() const                         { return header.userDefined; }

            bool                hasChild() const;
            bool                getFirstChild (DefElem *out) const;
            bool                getNextSibling (DefElem *out) const;

                                //this diventa il fratello di se stesso
            bool                next();
            bool                firstChild();


            u8                  structType_getNumMembers() const;

            u8                  arrayType_getNumDimension() const;
            u16                 arrayType_getNumElem (u8 index) const;
            bool                arrayType_isSimple() const;
            u8                  arrayType_getStride() const;


        private:
            void                priv_setup (const BufferR *reader, u16 startingPos, u16 endingPos);

        private:
            const BufferR   *reader;
            sElemHeader     header;
            u16             pos_curElem;
            u16             endingPos;

        friend DefReader;
        };

        typedef void (*trapFn_printOtherInfoOnThisRow)(UTF8String &out, const DefElem &elem);


        bool    blobDef_isValidMagic (const void *dataBlodDef);
        u16     blobDef_getSize (const void *dataBlodDef);
        u16     blobDef_getSizeOfDataBlob (const void *dataBlodDef);
        void    blobDef_prinfInfo (gos::UTF8String &out, const void *dataBlobDef, trapFn_printOtherInfoOnThisRow trapFn = NULL);

        /**
         * @brief   alloca un nuovo <dataBlob> le cui dimensioni in memoria dipendono
         *          da <dataBlobDef>
         */
        u8*     createNew (gos::Allocator *allocator, const void *dataBlobDef);


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
                    //
                    //La variante ..atOffset consente di specificare un preciso offset al quale fare partire l'elemento
                    //E' mandatorio che l'offset in questione sia sempre crescente, non e' possibile aggiungere elementi
                    //con offset minore dell'ultimo elemento aggiunto
            DefBuilder&     add_simpleType (const char *var_name, eDataFormat fmt, u32 userDefinedData=0, u32 paddedSize=u32MAX)                        { return add_simpleTypeAtOffset (sizeof_dataBlob, var_name, fmt, userDefinedData, paddedSize); }
            DefBuilder&     add_simpleTypeAtOffset (u16 offset, const char *var_name, eDataFormat fmt, u32 userDefinedData=0, u32 paddedSize=u32MAX);

            DefBuilder&     struct_begin (const char *var_name, u32 userDefinedData=0)                          { return struct_beginAtOffset (sizeof_dataBlob, var_name, userDefinedData); }
            DefBuilder&     struct_beginAtOffset (u16 offset, const char *var_name, u32 userDefinedData=0);
                            //add_simpleType..
                            //...
                            //add_simpleType..
            DefBuilder&     struct_end();

            DefBuilder&     array_begin1D (const char *var_name, u16 numElem1, u32 userDefinedData=0)                               { return array_begin1DAtOffset (sizeof_dataBlob, var_name, numElem1, userDefinedData); }
            DefBuilder&     array_begin1DAtOffset (u16 offset, const char *var_name, u16 numElem1, u32 userDefinedData=0);
            DefBuilder&     array_begin2D (const char *var_name, u16 numElem1, u16 numElem2, u32 userDefinedData=0)                 { return array_begin2DAtOffset (sizeof_dataBlob, var_name, numElem1, numElem2, userDefinedData); }
            DefBuilder&     array_begin2DAtOffset (u16 offset, const char *var_name, u16 numElem1, u16 numElem2, u32 userDefinedData=0);
            DefBuilder&     array_begin3D (const char *var_name, u16 numElem1, u16 numElem2, u16 numElem3, u32 userDefinedData=0)   { return array_begin3DAtOffset (sizeof_dataBlob, var_name, numElem1, numElem2, numElem3, userDefinedData); }
            DefBuilder&     array_begin3DAtOffset (u16 offset, const char *var_name, u16 numElem1, u16 numElem2, u16 numElem3, u32 userDefinedData=0);
                            //add_simpleType..
                            //...
                            //add_simpleType..
            DefBuilder&     array_end ();

            bool            end();

            bool            isValid() const;

                            //ritorna la dimensione inbyte dell'interno DataBlobDef
            u16             getDataBlobDefSize() const                  { return buffer.readU16At (4); }
            
                            //memcpia la DataBlobDef appena costruita in un generico buffer
            bool            memcpyDataBlobDef (void *dst, u32 sizeof_dst) const;

            u8*             allocDataBlobDef (gos::Allocator *allocator) const;
                            //alloca un buffer della necessaria dimensione e ci memcpia la DataBlobDef

        private:
            static constexpr u8     FLAG__BEGIN = 0;
            static constexpr u8     FLAG__ERROR = 1;

        private:
            u16             priv_elem_begin (eDataBlobElemType elemtype, const char *name, u32 userDefinedData);
            u16             priv_elem_end ();
            void            priv_add_pad();
            u16             priv_array_begin_start (u16 offset, const char *var_name, u8 numDimension, u32 userDefinedData);
            void            priv_array_begin_end(u16 pos_posOfFirstChild);

        private:
            gos::LIFOFixedSize<u16, 32> stack;
            gos::BufferW_linear         buffer;
            u16                         sizeof_dataBlob;
            gos::Flag8                  flag;
        }; //class DefBuilder

        
        /**
        * @brief DefReader
        * Classe di comodo utilizzata per accedere alla struttura implicata da una <dataBlobDef>
        */
        class DefReader
        {
        public:
                        DefReader()                                     { }
                        ~DefReader()                                    { }

            bool        setup (const void *dataBlobDef);
            u16         dataBlob_getSize() const                        { return reader.readU16At (6); }

            void        beginEnumerate (DefElem *out) const;

                        /**
                         * @param   var_name accetta la notazione 'dotted' come ad esempio "pippo.pluto"
                         *          Accetta anche la notazione [] per gli array (es: pippo,pluto[2] e pippo,pluto[2].m1)
                         */
            bool        getOffset (const char *var_name, u16 *out) const;

        private:
            BufferR     reader;
        }; //class DefReader


        /**
        * @brief Var
        * E' una coppia <dataBlobDef, dataBlob> e fornisce i metodi per lettura
        * e scrittura del <dataBlob>
        * 
        * Internamente mantiene un puntatore a un DefReader e a un dataBlob.
        * Non e' responsabilita' di questta classe fare alloc/free di <dataBlob> e <DefReader>.
        * Il DefReader deve essere stato inizializzato con il <dataBlobDef> adeguato per il <dataBlob>
        */
        class Var
        {
        public:
                    Var()                                                           { reader = NULL; dataBlob = NULL; }
                    ~Var()                                                          { }
                        
            void    setup (const DefReader *readerIN, u8 *dataBlobIN)               { reader = readerIN; dataBlob = dataBlobIN; }
            bool    getOffset (const char *var_name, u16 *out) const                { return reader->getOffset(var_name, out); }

            void    zero()                                                          { memset (dataBlob, 0, reader->dataBlob_getSize()); }
            void    rawset (u16 offset, const void *data, u32 sizeof_data)          { memcpy (&dataBlob[offset], data, sizeof_data); }
            void    rawget (u16 offset, void *dst, u32 howManyByteToRead) const     { memcpy (dst, &dataBlob[offset], howManyByteToRead); }

                    template<class T>
            void    set (u16 offset, const T &val)                                  { rawset (offset, &val, sizeof(T)); }

                    template<class T>
            void    set (const char *var_name, const T &val)
                    { 
                        u16 offset;
                        if (getOffset(var_name, &offset))
                            set<T>(offset, val);
#ifdef _DEBUG
                        else
                            DBGBREAK;
#endif
                    }

                    template<class T>
            T&      get (u16 offset) const								            { return * reinterpret_cast<T*>(&dataBlob[offset]); }

                    template<class T>
            T&      get (const char *var_name) const
                    {
                        u16 offset;
                        if (getOffset(var_name, &offset))
                            return get<T>(offset);

                        DBGBREAK;
                        return get<T>((u16)0);
                    }
            
            const DefReader*    getDef() const                                      { return reader; }
            u8*                 getDataBlob() const                                 { return dataBlob;} 

        private:
            const DefReader *reader;
            u8              *dataBlob;
        }; //class Var
      
    } //namespace datablob
} //namespace gos

#endif //_gosDataBlob_h_