#ifndef _gosSkeleton_h_
#define _gosSkeleton_h_
#include "gosEngineEnumAndDefine.h"

namespace gos
{
    /*******************************
     * @brief   Bone
     * 
     */
    struct Bone
    {
    public:
        Bone*   addChild (Bone *b);
        Bone*   addSibling (Bone *b);

    public:
        mat4x4f matrix;
        Bone    *firstChild;
        Bone    *sibling;
    };

    /*******************************
     * @brief   Skeleton
     * 
     */
    class Skeleton
    {
    public:
                    Skeleton()          { root = NULL; }
                    ~Skeleton()         { free(); }

        Bone*       createRoot (gos::Allocator *allocator);
        Bone*       newBone();
        void        free();

        const Bone* getRoot() const     { return root; }

    private:
        void        priv_free_ric (Bone *b);

    private:
        gos::Allocator  *allocator;
        Bone            *root;
    };


} //namespace gos

#endif //_gosSkeleton_h_

