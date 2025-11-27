#include "gosEntityDefaultComponents.h"
#include "../gosGeom/gosGeomEular.h"

using namespace gos;
using namespace ent;


//**************************************
void CompPos::buildMatrix (gos::mat4x4f *out)
{
    assert (NULL != out);
    gos::mat4x4f matS;
    matS.buildScale(scale);
   
    gos::mat4x4f matR;
    quat.toMatrix4x4  (&matR);
    matR = matR * matS;

    out->buildTranslation (pos);
    (*out) = (*out) * matR;
}