#include "characterController.h"
#include "../../gos/gosUtils.h"
#include "../../gosGeom/gosGeomEular.h"

using namespace gos;

//**************************************
CharacterController::CharacterController ()
{
    target_ent.setInvalid();
    target_cam = NULL;

    status.zero();
    setLinearSpeed (4);
    setRotationalSpeed (0.5f);
    lastTimeUpdated_msec = 0;
    rel_rotX_rad = rel_rotY_rad = 0;
}

//**************************************
CharacterController::~CharacterController ()
{}

//**************************************
void CharacterController::bind (gos::Entity ent, gos::geom::Camera3 *cam)
{ 
    target_ent = ent;
    target_cam = cam;
    rel_rotX_rad = rel_rotY_rad = 0;
}

//**************************************
void CharacterController::priv_setStatus (u16 MASK, bool b)
{
    if (b)
        status.set (MASK);
    else
        status.clear (MASK);
}

//**************************************
void CharacterController::rotateX (bool bClockwise)
{
    if (bClockwise)
    {
        rel_rotX_rad += rotationalSpeed_rad;
        if (rel_rotX_rad > gos::math::gradToRad(80))
            rel_rotX_rad = gos::math::gradToRad(80);
    }
    else
    {
        rel_rotX_rad -= rotationalSpeed_rad;
        if (rel_rotX_rad < -gos::math::gradToRad(60))
            rel_rotX_rad = -gos::math::gradToRad(60);
    }
}

//**************************************
void CharacterController::rotateY (bool bClockwise)
{
    if (bClockwise)
        rel_rotY_rad += rotationalSpeed_rad;
    else
        rel_rotY_rad -= rotationalSpeed_rad;
}

//**************************************
void CharacterController::mouseRotateY (i32 num_pixel_mouse_was_moved)
{
    if (num_pixel_mouse_was_moved < 0)
        //clockwise
        rel_rotY_rad -= (f32) rotationalSpeed_rad;
    else
        rel_rotY_rad += (f32)rotationalSpeed_rad;
}

//**************************************
void CharacterController::mouseRotateX (i32 num_pixel_mouse_was_moved)
{
    //printf ("%d\n", num_pixel_mouse_was_moved);
    if (num_pixel_mouse_was_moved < 0)
    {
        //clockwise
        rel_rotX_rad -= (f32)rotationalSpeed_rad;
        if (rel_rotX_rad > gos::math::gradToRad(80))
            rel_rotX_rad = gos::math::gradToRad(80);
    }
    else
    {
        rel_rotX_rad += (f32)rotationalSpeed_rad;
        if (rel_rotX_rad < -gos::math::gradToRad(60))
            rel_rotX_rad = -gos::math::gradToRad(60);
    }        
}


//**************************************
void CharacterController::update (gos::ent::Registry &entRegistry,u64 timenow_msec)
{
    if (target_ent.isInvalid())
        return;

    ent::CompPos *ent_pos = entRegistry.get<ent::CompPos>(target_ent, true);
    if (NULL == ent_pos)
        return;

    if (0 == lastTimeUpdated_msec)
        lastTimeUpdated_msec = timenow_msec;
    const f32 timeElapsed_sec = (f32)(timenow_msec - lastTimeUpdated_msec) / 1000.0f;
    const f32 linearSpeed = speed_msec * timeElapsed_sec;
    lastTimeUpdated_msec = timenow_msec;

    //rotazione attuale
    gos::mat3x3f matR;
    ent_pos->eular_rot.y += rel_rotY_rad;
    geom::eular_clamp_0_DUEPI (&ent_pos->eular_rot);
    geom::eular_compute3x3Matrix (ent_pos->eular_rot, &matR);
    rel_rotX_rad = rel_rotY_rad = 0;

	const vec3f asseX = vec3f(matR(0,0), matR(1,0), matR(2,0));
    const vec3f asseY = vec3f(matR(0,1), matR(1,1), matR(2,1));
    const vec3f asseZ = vec3f(matR(0,2), matR(1,2), matR(2,2));


    //movimento
    if (status.isBitSet (STATUS_MOVING_FORWARD))
        ent_pos->pos += asseZ * linearSpeed;
    else if (status.isBitSet (STATUS_MOVING_BACKWARD))
        ent_pos->pos -= asseZ * linearSpeed;

    if (status.isBitSet (STATUS_MOVING_RIGHT))
        ent_pos->pos += asseX * linearSpeed;
    else if (status.isBitSet (STATUS_MOVING_LEFT))
        ent_pos->pos -= asseX * linearSpeed;

    //if (status.isBitSet (STATUS_MOVING_UP))
    //    pos.moveRelAlongY (linearSpeed);
    //else if (status.isBitSet (STATUS_MOVING_DOWN))
    //    pos.moveRelAlongY (-linearSpeed);


    target_cam->pos.o =  (ent_pos->pos - asseZ * 20.0f + asseY * 20.0f);
    target_cam->pos.lookAt (ent_pos->pos);
    target_cam->markUpdated();

}

