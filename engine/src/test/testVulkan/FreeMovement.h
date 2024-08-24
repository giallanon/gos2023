#ifndef _FreeMovement_h_
#define _FreeMovement_h_
#include "../../gosGeom/gosGeomPos3.h"

/**************************************************
 * FreeMovement
 * t
*/
class FreeMovement
{
public:
            FreeMovement ();

    void    bind (gos::geom::Pos3 *pos);
    void    setLinearSpeed (f32 m_sec)              { speed_msec = m_sec; }
    void    setRotationalSpeed (f32 grad)           { rotationalSpeed_rad = gos::math::gradToRad(grad); }

    void    halt()                                  { status = 0; }
    void    moveForward (bool b)                    { priv_setStatus (STATUS_MOVING_FORWARD, b); }
    void    moveBackward (bool b)                   { priv_setStatus (STATUS_MOVING_BACKWARD, b); }
    void    strafeLeft (bool b)                     { priv_setStatus (STATUS_MOVING_LEFT, b); }
    void    strafeRight (bool b)                    { priv_setStatus (STATUS_MOVING_RIGHT, b); }
    void    strafeUp (bool b)                       { priv_setStatus (STATUS_MOVING_UP, b); }
    void    strafeDown (bool b)                     { priv_setStatus (STATUS_MOVING_DOWN, b); }    
    void    rotateY (bool bClockwise);
    void    rotateX (bool bClockwise);

    void    update (u64 timenow_msec);

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
    gos::geom::Pos3     *targetPos;
    f32                 speed_msec;
    f32                 rotationalSpeed_rad;
    u64                 lastTimeUpdated_msec;
    u16                 status;
};

#endif // _FreeMovement_h_