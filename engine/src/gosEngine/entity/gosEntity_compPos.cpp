#include "gosEntityEnumAndDefine.h"

using namespace gos;
using namespace ent;


//**************************************
void CompPos::updateMatrix()
{
    gos::mat4x4f matS, matR;

    matS.buildScale(scale);

    if (rot_grad.x > 360.0f)   rot_grad.x -= 360.0f;
    if (rot_grad.x < 0.0f)     rot_grad.x += 360.0f;
    if (rot_grad.y > 360.0f)   rot_grad.y -= 360.0f;
    if (rot_grad.y < 0.0f)     rot_grad.y += 360.0f;
    if (rot_grad.z > 360.0f)   rot_grad.z -= 360.0f;
    if (rot_grad.z < 0.0f)     rot_grad.z += 360.0f;
    matR.buildFromEulerAngles_YXZ (math::gradToRad(rot_grad.y), math::gradToRad(rot_grad.x), math::gradToRad(rot_grad.z) );
    matR = matR * matS;


    _matrix.buildTranslation (pos);
    _matrix = _matrix * matR;
}