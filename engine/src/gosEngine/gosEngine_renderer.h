#ifndef _gosEngine_renderer_h_
#define _gosEngine_renderer_h_
#include "gosEngineEnumAndDefine.h"
#include "../gosGeom/gosGeomCamera3.h"


namespace gos
{
    namespace engine
    {
        class Renderer
        {
        public:
                    Renderer();
                    ~Renderer()                                                                                     { }

            void    begin (gos::geom::Camera3 *cam);
            void    add (const ENGShape shape, const ENGMatrixW worldPos);
            void    end();
        private:

        };
    } //namespace engine
} //namespace gos


#endif //_gosEngine_renderer_h_
