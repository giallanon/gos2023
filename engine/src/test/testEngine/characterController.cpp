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
    cam_rotX_rad = rel_rotY_rad = 0;
}

//**************************************
CharacterController::~CharacterController ()
{}

//**************************************
void CharacterController::bind (gos::Entity ent, gos::geom::Camera3 *cam)
{ 
    target_ent = ent;
    target_cam = cam;
    cam_rotX_rad = rel_rotY_rad = 0;
    camera_distance_m = 20.0f;
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
void CharacterController::camera_rotate_aboutY (bool bClockwise)
{
    if (bClockwise)
        //clockwise
        rel_rotY_rad -= (f32) rotationalSpeed_rad;
    else
        rel_rotY_rad += (f32)rotationalSpeed_rad;
}

//**************************************
void CharacterController::camera_rotate_aboutX (bool bClockwise)
{
    //printf ("%d\n", num_pixel_mouse_was_moved);
    if (bClockwise)
    {
        //clockwise
        cam_rotX_rad -= (f32)rotationalSpeed_rad;
    }
    else
    {
        cam_rotX_rad += (f32)rotationalSpeed_rad;
    }        
}

//**************************************
void CharacterController::camera_adjust_distance (bool bIncreaseDistance)
{
    if (bIncreaseDistance)
        camera_distance_m += 0.5f;
    else
        camera_distance_m -= 0.5f;

    if (camera_distance_m < 2.0f)
        camera_distance_m = 2.0f;
    if (camera_distance_m > 80.0f)
        camera_distance_m = 80.0f;
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
    gos::Quat quat;
    ent_pos->quat.rotateMeAbout (vec3f(0,1,0), rel_rotY_rad);
    rel_rotY_rad = 0;


	vec3f asseX, asseY, asseZ;
    ent_pos->quat.toAxis (&asseX, &asseY, &asseZ);


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


  
    if (cam_rotX_rad < math::gradToRad(15))
        cam_rotX_rad = math::gradToRad(15);
    if (cam_rotX_rad > math::gradToRad(80))
        cam_rotX_rad = math::gradToRad(80);

    target_cam->pos.o = ent_pos->pos;
    target_cam->pos.setFromQuat (ent_pos->quat);
    target_cam->pos.rotateMeAboutMyX (-cam_rotX_rad);
    target_cam->pos.moveRelAlongZ (-camera_distance_m);
    target_cam->markUpdated();
}

