#include "gosGameUtils.h"

using namespace gos;

//questo indica quanti bit usare per la componente x e quanti per la y
static constexpr u32 OCTAHEDRAL_PRECISION_BIT = 16;

//*************************************
u32	utils::normal_encode_octahedral (vec3f norm)
{
    norm /= ( math::abs(norm.x) + math::abs(norm.y) + math::abs(norm.z) );

	if (norm.z < 0.0f)
	{
		//norm.xy = (1.0 - fabs(norm.yx)) * sign(norm.xy);
		norm.x = (1.0f - math::abs(norm.y)) * math::sign(norm.x);
		norm.y = (1.0f - math::abs(norm.x)) * math::sign(norm.y);
	}
    
	//vec2f v = 0.5f + 0.5f * norm.xy;
	const vec2f v (0.5f + 0.5f * norm.x, 0.5f + 0.5f * norm.y);

    const f32 mu = (f32) ( (1u << OCTAHEDRAL_PRECISION_BIT) - 1u );
    //uvec2 d = uvec2(floor(v*float(mu)+0.5));
	const vec2u d (
		(u32)math::floor( v.x * mu + 0.5f),
		(u32)math::floor( v.y * mu + 0.5f)
	);

    return (d.y << OCTAHEDRAL_PRECISION_BIT) | d.x;
}

//*************************************
vec3f utils::normal_decode_octahedral (u32 encoded_value, bool bNormalizeResult)
{
    const u32 mu = (1u << OCTAHEDRAL_PRECISION_BIT) - 1u;

    //uvec2 d = uvec2( encoded_value, encoded_value >> OCTAHEDRAL_PRECISION_BIT ) & mu;
	const vec2u d ( encoded_value  & mu, 
					(encoded_value >> OCTAHEDRAL_PRECISION_BIT)  & mu );

    //vec2 v = vec2(d)/float(mu);
	vec2f v (  (f32)d.x /(f32)mu,  (f32)d.y/(f32)mu  );
    //v = -1.0f + 2.0f*v;
	v.x = -1.0f + 2.0f * v.x;
	v.y = -1.0f + 2.0f * v.y;

    // Rune Stubbe's version
    //vec3 nor = vec3(v, 1.0 - abs(v.x) - abs(v.y));
	vec3f norm ( v.x, v.y, 1.0f - math::abs(v.x) - math::abs(v.y));
    const f32 t = GOSMAX(-norm.z, 0.0f);
    norm.x += (norm.x>0.0) ? -t : t;
    norm.y += (norm.y>0.0) ? -t : t;

	if (bNormalizeResult)
		norm.normalize();
	return norm;
}
