#ifndef _characterController_h_
#define _characterController_h_
#include "entity/gosEntity.h"



class CharacterController
{
public:
	        CharacterController();
	        ~CharacterController();


	void    bind (gos::Entity ent, gos::geom::Camera3 *cam);
    void    setLinearSpeed (f32 m_sec)              { speed_msec = m_sec; }
    void    setRotationalSpeed (f32 grad)           { rotationalSpeed_rad = gos::math::gradToRad(grad); }

    void    halt()                                  { status.zero(); }
    void    moveForward (bool b)                    { priv_setStatus (STATUS_MOVING_FORWARD, b); }
    void    moveBackward (bool b)                   { priv_setStatus (STATUS_MOVING_BACKWARD, b); }
    void    strafeLeft (bool b)                     { priv_setStatus (STATUS_MOVING_LEFT, b); }
    void    strafeRight (bool b)                    { priv_setStatus (STATUS_MOVING_RIGHT, b); }
    void    rotateY (bool bClockwise);
    void    rotateX (bool bClockwise);

    void    camera_adjust_distance (bool bIncreaseDistance);
    void    camera_rotate_aboutX (bool bClockwise);
    void    camera_rotate_aboutY (bool bClockwise);

    void    update (gos::ent::Registry &entRegistry, u64 timenow_msec);

private:
    const u16   STATUS_MOVING_FORWARD  = 0;
    const u16   STATUS_MOVING_BACKWARD = 1;
    const u16   STATUS_MOVING_LEFT = 2;
    const u16   STATUS_MOVING_RIGHT = 3;
    const u16   STATUS_MOVING_UP = 4;
    const u16   STATUS_MOVING_DOWN = 5;

private:
    void    priv_setStatus (u16 MASK, bool b);

private:
    gos::Entity         target_ent;
    gos::geom::Camera3  *target_cam;

    f32                 speed_msec;
    f32                 rotationalSpeed_rad;
    u64                 lastTimeUpdated_msec;
    gos::Flag16         status;
    f32                 camera_distance_m;
    f32                 cam_rotX_rad;
    f32                 rel_rotY_rad;
};

#endif //_characterController_h_