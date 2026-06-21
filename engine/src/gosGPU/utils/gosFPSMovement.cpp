#include "gosFPSMovement.h"
#include "../../gos/gosUtils.h"

using namespace gos;

//**************************************
FPSMovement::FPSMovement ()
{
    targetPos = NULL;
    status.zero();
    setLinearSpeed (4);
    setRotationalSpeed (0.5f);
    lastTimeUpdated_msec = 0;
    rotX_rad = rotY_rad = 0;
}

//**************************************
void FPSMovement::bind (gos::geom::Pos3 *posIN)
{ 
    targetPos = posIN;

    gos::vec3f p = posIN->o + (posIN->getAsseZ() * 4.0f);
    pos.identity();
    pos.warp (posIN->o);
    pos.lookAt (p);
}

//**************************************
void FPSMovement::priv_setStatus (u16 MASK, bool b)
{
    if (b)
        status.set (MASK);
    else
        status.clear (MASK);
}

//**************************************
void FPSMovement::rotateX (bool bClockwise)
{
    if (bClockwise)
    {
        rotX_rad += rotationalSpeed_rad;
        if (rotX_rad > gos::math::gradToRad(80))
            rotX_rad = gos::math::gradToRad(80);
    }
    else
    {
        rotX_rad -= rotationalSpeed_rad;
        if (rotX_rad < -gos::math::gradToRad(60))
            rotX_rad = -gos::math::gradToRad(60);
    }
}

//**************************************
void FPSMovement::rotateY (bool bClockwise)
{
    if (bClockwise)
        rotY_rad += rotationalSpeed_rad;
    else
        rotY_rad -= rotationalSpeed_rad;
}

//**************************************
void FPSMovement::mouseRotateY (i32 num_pixel_mouse_was_moved)
{
    if (num_pixel_mouse_was_moved < 0)
        //clockwise
        rotY_rad -= (f32) rotationalSpeed_rad;
    else
        rotY_rad += (f32)rotationalSpeed_rad;
}

//**************************************
void FPSMovement::mouseRotateX (i32 num_pixel_mouse_was_moved)
{
    //printf ("%d\n", num_pixel_mouse_was_moved);
    if (num_pixel_mouse_was_moved < 0)
    {
        //clockwise
        rotX_rad -= (f32)rotationalSpeed_rad;
        if (rotX_rad > gos::math::gradToRad(80))
            rotX_rad = gos::math::gradToRad(80);
    }
    else
    {
        rotX_rad += (f32)rotationalSpeed_rad;
        if (rotX_rad < -gos::math::gradToRad(60))
            rotX_rad = -gos::math::gradToRad(60);
    }        
}


//**************************************
void FPSMovement::update (u64 timenow_msec)
{
    if (NULL == targetPos)
        return;
    if (0 == lastTimeUpdated_msec)
        lastTimeUpdated_msec = timenow_msec;
    const f32 timeElapsed_sec = (f32)(timenow_msec - lastTimeUpdated_msec) / 1000.0f;
    const f32 linearSpeed = speed_msec * timeElapsed_sec;
    lastTimeUpdated_msec = timenow_msec;

    pos.setFromEulerAngles_YXZ (rotY_rad, 0, 0);
    if (status.isBitSet (STATUS_MOVING_FORWARD))
        pos.moveRelAlongZ (linearSpeed);
    else if (status.isBitSet (STATUS_MOVING_BACKWARD))
        pos.moveRelAlongZ (-linearSpeed);

    if (status.isBitSet (STATUS_MOVING_RIGHT))
        pos.moveRelAlongX (linearSpeed);
    else if (status.isBitSet (STATUS_MOVING_LEFT))
        pos.moveRelAlongX (-linearSpeed);

    if (status.isBitSet (STATUS_MOVING_UP))
        pos.moveRelAlongY (linearSpeed);
    else if (status.isBitSet (STATUS_MOVING_DOWN))
        pos.moveRelAlongY (-linearSpeed);
   

    targetPos->setFromEulerAngles_YXZ (rotY_rad, rotX_rad, 0);
    targetPos->warp (pos.o);
}

