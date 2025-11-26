#include "gosEntityEnumAndDefine.h"
#include "../gosGeom/gosGeomEular.h"

using namespace gos;
using namespace ent;


//**************************************
void CompPos::updateMatrix()
{
    gos::mat4x4f matS;
    matS.buildScale(scale);
   
    gos::mat4x4f matR;
    geom::eular_clamp_0_DUEPI (&eular_rot);
    geom::eular_compute4x4Matrix (eular_rot, &matR);
    
    matR = matR * matS;

    _matrix.buildTranslation (pos);
    _matrix = _matrix * matR;
}