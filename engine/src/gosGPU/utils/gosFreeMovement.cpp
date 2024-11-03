#include "gosFreeMovement.h"
#include "../../gos/gosUtils.h"

using namespace gos;

//**************************************
FreeMovement::FreeMovement ()
{
    targetPos = NULL;
    status.zero();
    rotX_rad = rotY_rad = rotZ_rad = 0;
    setLinearSpeed (4);
    setRotationalSpeed (0.5f);
    lastTimeUpdated_msec = 0;
}

//**************************************
void FreeMovement::bind (gos::geom::Pos3 *posIN)
{ 
    targetPos = posIN;
    targetPos->getEulerAngles_YXZ (&rotY_rad, &rotX_rad, &rotZ_rad);
    rotZ_rad = 0;
}

//**************************************
void FreeMovement::priv_setStatus (u16 MASK, bool b)
{
    if (b)
        status.set (MASK);
    else
        status.clear (MASK);
}

//**************************************
void FreeMovement::rotateX (bool bClockwise)
{
    static constexpr f32 LIMIT = gos::math::gradToRad(179);
    if (bClockwise)
    {
        rotX_rad += rotationalSpeed_rad;
        if (rotX_rad > LIMIT)
            rotX_rad = LIMIT;
    }
    else
    {
        rotX_rad -= rotationalSpeed_rad;
        if (rotX_rad < -LIMIT)
            rotX_rad = -LIMIT;
    }
}        


//**************************************
void FreeMovement::rotateY (bool bClockwise)
{
    if (bClockwise)
        rotY_rad += rotationalSpeed_rad;
    else
        rotY_rad -= rotationalSpeed_rad;        
}

//**************************************
void FreeMovement::update (u64 timenow_msec)
{
    if (NULL == targetPos)
        return;
    if (0 == lastTimeUpdated_msec)
        lastTimeUpdated_msec = timenow_msec;
    const f32 timeElapsed_sec = (f32)(timenow_msec - lastTimeUpdated_msec) / 1000.0f;
    const f32 linearSpeed = speed_msec * timeElapsed_sec;
    lastTimeUpdated_msec = timenow_msec;


    targetPos->setFromEulerAngles_YXZ (rotY_rad, rotX_rad, rotZ_rad);
    if (status.isBitSet (STATUS_MOVING_FORWARD))
        targetPos->moveRelAlongZ (linearSpeed);
    else if (status.isBitSet (STATUS_MOVING_BACKWARD))
        targetPos->moveRelAlongZ (-linearSpeed);

    if (status.isBitSet (STATUS_MOVING_RIGHT))
        targetPos->moveRelAlongX (linearSpeed);
    else if (status.isBitSet (STATUS_MOVING_LEFT))
        targetPos->moveRelAlongX (-linearSpeed);

    if (status.isBitSet (STATUS_MOVING_UP))
        targetPos->moveRelAlongY (linearSpeed);
    else if (status.isBitSet (STATUS_MOVING_DOWN))
        targetPos->moveRelAlongY (-linearSpeed);

}

