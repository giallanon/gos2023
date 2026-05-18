#ifndef _gosSkeleton_h_
#define _gosSkeleton_h_
#include "../gosShapeEnumAndDefine.h"
#include "../../gos/gos.h"
#include "../../gos/string/gosStringList.h"
#include "../../gos/string/gosUTF8String.h"

namespace gos
{
    /*******************************
     * @brief   Skeleton
     * 
     * E' un blob di memoria formattato come segue:
     *      0       u32 magic
     *      4       u32 total_size_of_blob
     *      8       u8  num_bone
     *      9       pad
     *      10      u16 abs-offset to START-OF-NAME-TABLE
     *      12      sizeof(Bone)    bone-0
     *      ..
     *      ..      sizeof(Bone)    bone-N
     * 
     *      ..      u16 abs-offset to name-0       //START-OF-NAME-TABLE
     *      ..
     *      ..      u16 abs-offset to for name-N
     *      ..
     *      ..      name-0 (stringa con 0x0 come terminatore)
     *      ..
     *      ..      name-N (stringa con 0x0 come terminatore)
     */
    struct Skeleton
    {
    public:
        gos::Allocator  *allocator;
        u8              *blob;

	public:
		void    reset()		{ allocator=NULL; blob=NULL; }
    };


    /*******************************
     * @brief   Bone
     * 
     */
    struct Bone
    {
    public:
        mat4x4f matrix;
        u8      firstChildIndex;
        u8      sigblinIndex;
        u16     bone_index; //index di questa bone all'interno del suo skeleton
    };


    namespace skeleton
    {
        bool    isValid (const Skeleton &sk);
        void    free (Skeleton &sk);

		u32 	get_blob_size (const Skeleton &sk);
        
				//se [buffer] == NULL ritorna il num di byte necessari alla serializzazione
				//se [buffer] != NULL ritorna 0 in caso di errore oppure il num di byte memcpyati in [buffer]
        u32     serialize (const Skeleton &sk, u8 *buffer, u32 sizeof_buffer);

				//ritorna 0 in caso di errore
				//altrimenti ritorna il num di byte consumati per la deserializzazione.
				//[allocator] e' utilizzato per le strutture interne di skeleton
        u32     deserialize (const u8 *buffer, u32 sizeof_buffer, gos::Allocator *allocator, Skeleton *out);
    
        void    debug__print (const Skeleton &sk, gos::UTF8String &out);

		



        /*******************************
         * @brief   SkeletonBuilder
         *          Classe di comodo per la costruzione di Skeleton
         */
        class Builder
        {
        public:    
                    Builder();
                    ~Builder();

            u32     begin (const char *rootName, Bone **out_canBeNULL = NULL);

                    //crea una nuova Bone con nome <dstBoneName> e la adda come figlio di <srcBoneIndex>.
                    //Ritorna l'index della nuova bone creata
            u32     addChildTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL = NULL);

                    //crea una nuova Bone con nome <dstBoneName> e la adda come fratello di <srcBoneIndex>.
                    //Ritorna l'index della nuova bone creata
            u32     addSiblingTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL= NULL);

            bool    end (gos::Allocator *allocator, Skeleton *out);

        private:
            u32     priv_newBone (const char *name);

        private:
            gos::StringList         nameList;
            gos::FastArray<Bone>    boneList;
        };    



        /*******************************
         * @brief   SkeletonReader
         *          Classe di comodo per la lettura delle info di uno skeleton
         */
        class Reader
        {
        public:
                        Reader()                        { sk = NULL; }
                        Reader(const Skeleton *sk)      { setup(sk); }
                        ~Reader()                       { }

            void        setup (const Skeleton *sk);
            u32         bone_get_num() const;
            const Bone* bone_get_by_index (u32 bone_index) const;
            const Bone* bone_get_by_name (const char *s) const;
            u32         bone_get_index_by_name (const char *s) const;
            const char* name_get_by_index (u32 bone_index) const;
            const char* name_get_by_bone (const Bone *bone) const;

        private:
            const Skeleton *sk;
        };

    } //namespace skeleton
    
    
} //namespace gos

#endif //_gosSkeleton_h_

