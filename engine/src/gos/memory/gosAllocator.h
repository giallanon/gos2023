#ifndef _gosAllocator_h_
#define _gosAllocator_h_
#include "../gosEnumAndDefine.h"

namespace gos
{
    /*==============================================================
    * Allocator
    *
    * Interfaccia di base per tutti gli allocatori di memoria
    */
    class Allocator
    {
    public:
                            Allocator (const char *nameIN)									
                            { 
                                myID = allocatorID++; 
                                if (NULL == nameIN)
                                    sprintf_s (name, sizeof(name),"allocator%d", myID);
                                else
                                    strcpy_s (name, sizeof(name), nameIN);
                            }

        virtual				~Allocator() { }
    
#ifdef _DEBUG
                            //[bPlacementNew] e' piu' che altro per questioni di debug e serve ad indicare se la alloc si sta
                            //utilzzando per instanziare una classe (true) oppure se e' una plain malloc (false).
                            //Cosi' facendo, durante la dealloc() posso accertarmi che le cose allocate con GOSNEW siano deallocate con GOSDELETE e tutto il resto
                            //invece con GOSFREE
        static constexpr u8	DEBUG_HEADER_SIZE = 8;

        void*				alloc (size_t sizeInBytes, u8 alignPowerOfTwo, UNUSED_PARAM(const char *debug_filename), u32 debug_lineNumber, bool debug_bPlacementNew)					
                            { 
                                assert(GOS_IS_POWER_OF_TWO(alignPowerOfTwo));
                                sizeInBytes += DEBUG_HEADER_SIZE;
                                void *ret = virt_do_alloc(sizeInBytes, alignPowerOfTwo);
                                if (ret)
                                    memset(ret, 0xCA, sizeInBytes);

                                u32 *p32 = reinterpret_cast<u32*>(ret);
                                p32[0] = debug_lineNumber;

                                u8 *p8 = reinterpret_cast<u8*>(ret);
                                if (debug_bPlacementNew)
                                    p8[4] = 0x01;
                                else
                                    p8[4] = 0x02;

                                p8 += DEBUG_HEADER_SIZE;
                                return p8;
                            }

        void				dealloc (void *p, bool bPlacementNew)
                            { 
                                u8 *p8 = reinterpret_cast<u8*>(p);
                                p8 -= DEBUG_HEADER_SIZE;

                                //const u32 *p32 = reinterpret_cast<const u32*>(p8);
                                //u32 debug_lineNumber = p32[0];		//linea di codice dove e' avvenuta l'allocazione

                                //se e' stato allocato con GOSNEW, deve essere liberato con GOSDELETE:
                                //Se allocato con GOSALLOC, deve essere liberato con GOSFREE
                                if (bPlacementNew)			
                                    assert (p8[4] == 0x01);
                                else
                                    assert (p8[4] == 0x02);

                                virt_do_dealloc(p8); 
                            }

#else
        void*				alloc (size_t sizeInBytes, u8 alignPowerOfTwo)      { return virt_do_alloc(sizeInBytes, alignPowerOfTwo); }									
        void				dealloc (void *p)                                   { virt_do_dealloc(p); }

#endif

        const char*			getName() const										{ return name; }
        u16					getAllocatorID() const								{ return myID; }

        virtual	bool		isThreadSafe() const = 0;
        virtual size_t		getAllocatedSize (const void *p) = 0;


    protected:
        virtual void*		virt_do_alloc (size_t sizeInBytes, u8 alignPowerOfTwo) = 0;
        virtual void		virt_do_dealloc (void *p) = 0;

    private:
        static u16			allocatorID;

    private:
        char                name[32];
        u16					myID;
    };
} //namespace gos

#endif //_gosAllocator_h_
