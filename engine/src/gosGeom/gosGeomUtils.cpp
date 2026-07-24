#include "gosGeomUtils.h"

using namespace gos;


//*********************************************
void geom::circle (FastArray<vec3f> *out_vtxList, const vec3f &center, f32 radius, u32 numPoint, f32 starting_angle)
{
    assert (NULL != out_vtxList);

    const f32 rad_incr = math::DUEPI / (f32)numPoint;
    f32 rad = math::gradToRad(starting_angle);

    while (numPoint--)
    {
        vec3f v;

        v.x = cosf(rad) * radius;
        v.y = sinf(rad) * radius;
        v.z = 0;
        v += center;

        out_vtxList->append(v);
        rad += rad_incr;
    }
}

//*********************************************
void geom::circle (FastArray<vec2f> *out_vtxList, const vec2f &center, f32 radius, u32 numPoint, f32 starting_angle)
{
    assert (NULL != out_vtxList);

    const f32 rad_incr = math::DUEPI / (f32)numPoint;
    f32 rad = math::gradToRad(starting_angle);

    while (numPoint--)
    {
        vec2f v;

        v.x = cosf(rad) * radius;
        v.y = sinf(rad) * radius;
        v += center;

        out_vtxList->append(v);
        rad += rad_incr;
    }
}

//*********************************************
static f32 geom_point2D_order_clockwise_calc_angle (const vec2f p)
{
	f32 ret = atan2f (p.y, p.x);
	if (ret < 0) ret += math::DUEPI;
	else if (ret >= math::DUEPI)	ret -= math::DUEPI;
	return ret;
}

void geom::point2D_order_clockwise (const vec2f center, const vec2f *point_list, u32 num_point, u32 *out_oder, u32 sizeof_out_order)
{
	static constexpr u32 NUM_MAX_POINT = 256;

	assert (sizeof_out_order >= num_point * sizeof(u32));
	assert (num_point <= NUM_MAX_POINT);

	f32 angle_list[NUM_MAX_POINT];
	for (u32 i=0; i<num_point; i++)
	{
		out_oder[i] = i;
		const vec2f p = point_list[i] - center;
		angle_list[i] = geom_point2D_order_clockwise_calc_angle (p);
	}

	bool bEsci = false;
	u32 n = num_point;
	while (bEsci == false)
	{
		bEsci = true;
		n--;
		
		for (u32 i = 0; i < n; i++)
		{
			const u32 idx1 = out_oder[i];
			const u32 idx2 = out_oder[i+1];
			if (angle_list[idx1] < angle_list[idx2])
			{
				bEsci = false;
				GOSSWAP(out_oder[i], out_oder[i+1]);
			}
		}
	}		
}