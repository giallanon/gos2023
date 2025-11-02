#ifndef _gosEngine_scene_h_
#define _gosEngine_scene_h_
#include "gosEngineEnumAndDefine.h"
#include "entity/gosEntity.h"

namespace gos
{
    namespace engine
    {
        class Scene
        {
        public:
                        Scene()                                             { allocator = NULL; }
                        ~Scene()                                            { unsetup(); }

            void        setup (gos::Allocator *allocator);
            void        unsetup();

            void        begin();
            void        add (Entity ent);
            void        end();

            void        query (geom::Camera3 &cam, ent::UniqueList *out_list, bool bClearList = true) const;


        private:
            gos::Allocator          *allocator;
            gos::FastArray<Entity>  entityList;
        };

    } //namespace engine
} //namespace gos
#endif //_gosEngine_scene_h_