#include "FreeMovement.h"
#include "../../gos/gosUtils.h"

//**************************************
FreeMovement::FreeMovement ()
{
    targetPos = NULL;
    status = 0;
    setLinearSpeed (4);
    setRotationalSpeed (0.5f);
    lastTimeUpdated_msec = 0;
}

//**************************************
void FreeMovement::bind (gos::geom::Pos3 *posIN)
{ 
    targetPos = posIN;
}

//**************************************
void FreeMovement::priv_setStatus (u16 MASK, bool b)
{
    if (b)
        gos::utils::bitSET (&status, MASK);
    else
        gos::utils::bitCLEAR (&status, MASK);
}

//**************************************
void FreeMovement::rotateX (bool bClockwise)
{
    if (bClockwise)
        targetPos->rotateMeAboutMyX (rotationalSpeed_rad);
    else
        targetPos->rotateMeAboutMyX (-rotationalSpeed_rad);
}

//**************************************
void FreeMovement::rotateY (bool bClockwise)
{
    if (bClockwise)
        targetPos->rotateMeAboutMyY (rotationalSpeed_rad);
    else
        targetPos->rotateMeAboutMyY (-rotationalSpeed_rad);
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

    if (gos::utils::isBitSET (&status, STATUS_MOVING_FORWARD))
        targetPos->moveRelAlongZ (linearSpeed);
    else if (gos::utils::isBitSET (&status, STATUS_MOVING_BACKWARD))
        targetPos->moveRelAlongZ (-linearSpeed);

    if (gos::utils::isBitSET (&status, STATUS_MOVING_RIGHT))
        targetPos->moveRelAlongX (linearSpeed);
    else if (gos::utils::isBitSET (&status, STATUS_MOVING_LEFT))
        targetPos->moveRelAlongX (-linearSpeed);

    if (gos::utils::isBitSET (&status, STATUS_MOVING_UP))
        targetPos->moveRelAlongY (linearSpeed);
    else if (gos::utils::isBitSET (&status, STATUS_MOVING_DOWN))
        targetPos->moveRelAlongY (-linearSpeed);

}

