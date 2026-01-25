#ifndef _gosEngineLoaders_h_
#define _gosEngineLoaders_h_
#include "../../gosAsset2/gosAsset2.h"
#include "../../gos/gosBufferReader.h"
#include "../../gos/gosDataBlob.h"


namespace gos
{
	class Engine; //fwd
	
    namespace engine
    {
        namespace loaders
        {
            /*******************************************************
             * @brief   KnownAssets
             *          Mantiene una lista di <uid, pt_to_data> di tutti gli asset che sono stati caricati con successo,
             *          dove <pt_to_data> e' il membro 'Data' di una risorsa dell'engine (ad es: ResTexture::Data).
             *          E' un doppione perche' anche Engine tiene una lista degli asset, ma serve per caricare gli asset
             *          con dipendenza.
             *          Ad esempio, una pipe viene schedulata caricando vtx_shader, poi pxl_shader e poi la pipe stessa-
             *          Quando arriviamo a dove creare la pipe, mi servono vtx e pxl shader e non li posso chiedere all'engine
             *          per problemi di concorrenza ma anche per il fatto che l'engine potrebbe non essere ancora consapevole che
             *          vtx/pxl sono stati caricati e sono pronti (perche' non ha ancora processato il msg che LoaderThread gli ha
             *          mandato dicendo che gli shader sono ready).
             *          A questo punto mi serve poter accedere agli shader e lo faccio utilizzando KnownAssets * 
             */
            class KnownAssets
            {
            public:
                        KnownAssets()                               { allocator = NULL; }
                        ~KnownAssets()                              { unsetup(); }

                void    setup (gos::Allocator *allocatorIN)         { allocator = allocatorIN; list.setup (allocator, 8192); }
                void    unsetup()
                {
                    if (NULL == allocator)
                        return;
                    list.forEach ( [allocator=this->allocator](asset2::UID uid, Info value) {
                        GOSFREE(allocator, value.pt_to_data);
                        return true;
                    });

                    list.unsetup();
                    allocator = NULL;
                }
                
                void    add_or_replace (asset2::UID uid, const void *data, u32 sizeof_data)
                {
                    HashedUIDList::Position pos;
                    if (list.findPosition (uid, &pos))
                    {
                        Info *cur_value = list.getValueAtPos(pos);
                        assert (cur_value->allocated_size == sizeof_data);
                        memcpy (cur_value->pt_to_data, data, sizeof_data);
                    }
                    else
                    {
                        Info value;
                        value.allocated_size = sizeof_data;
                        value.pt_to_data = GOSALLOC(allocator, sizeof_data);
                        memcpy (value.pt_to_data, data, sizeof_data);
                        list.insertInPosition (pos, value);
                    }
                }

                void*   find_data_by_uid (asset2::UID uid) const
                {
                    Info info;
                    if (list.find (uid, &info))
                        return info.pt_to_data;
                    return NULL;
                }

            private:
                struct Info
                {
                    void    *pt_to_data;
                    u32     allocated_size;
                };

            private:
                gos::Allocator *allocator;
                typedef FastHashMap<asset2::UID, Info> HashedUIDList;

            private:
                HashedUIDList list;
            };



            //********************************************************
            struct LoaderInfo
            {
                gos::Allocator      *thread_allocator;
				gos::Allocator		*engine_allocator;
				Engine				*engine;
                gos::Logger			*logger;
                gos::GPU            *gpu;
                asset2::DBContext   *ctx;
                KnownAssets         *listof_knownAssets;
            };


            //********************************************************
            class BaseLoader
            {
            public:
                                BaseLoader()        { } 
                virtual         ~BaseLoader()       { }
                virtual bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *out_data) = 0;
            };
        } //namespace loaders
    } //namespace engine
} //namespace gos
#endif //_gosEngineLoaders_h_
