#extension GL_EXT_nonuniform_qualifier : require


//////////////////// descr-set 0
#define PIPE3_SAMPLER2D_BILINEAR  			0
#define PIPE3_SAMPLER2D_POINT     			1
layout(set = 0, binding = 0) uniform sampler   PIPE3_samplerList[2];
layout(set = 0, binding = 1) uniform texture2D PIPE3_textureList[];

//**** utils
vec3 color_sRGB_to_linear (uint r, uint g, uint b)
{
    vec3 ret;
    ret.x = pow (r/255.0f, 2.2f);
    ret.y = pow (g/255.0f, 2.2f);
    ret.z = pow (b/255.0f, 2.2f);

    return ret;
}

//**** sample di una texture 2D con bilinear filtering
vec4 PIPE3_sample2D_bilinear (const uint textureIndex, const vec2 texCoord)
{
    return texture (sampler2D(PIPE3_textureList[textureIndex], PIPE3_samplerList[PIPE3_SAMPLER2D_BILINEAR]), texCoord);
}

//**** sample di una texture 2D con point filtering
vec4 PIPE3_sample2D_point (const uint textureIndex, const vec2 texCoord)
{
    return texture (sampler2D(PIPE3_textureList[textureIndex], PIPE3_samplerList[PIPE3_SAMPLER2D_POINT]), texCoord);
}

/**** trasforma le normali
 E' necessario moltiplicare per la trasporta dell'inversa per tenere in conto
 eventuali operazioni di scaling non uniformi incluse nella matrice.
 Se ci sono solo operazioni di scaling uniformi, allora si puo' moltiplicare normalmente
 seza trasposta dell'inversa.
 Bisogna inoltre escludere la parte di "traslazione" e questo si fa convertendo dal 4x4
 in una 3x3*/
vec3 PIPE3_transform_normal_by_world_matrix4x4 (const vec3 norm, const mat4 matW)
{
    //return norm * mat3(matW);
    //return norm * mat3(transpose(inverse(matW)));
    return norm * (transpose(inverse(mat3(matW))));
}
vec3 PIPE3_transform_normal_by_world_matrix3x3 (const vec3 norm, const mat3 matW)
{
    //return norm * mat3(matW);
    return norm * transpose(inverse(matW));
}


//**** semplice calcolo luce
float PIPE3_calcLight_01 (const vec4 lightDir_and_ambient, const vec3 norm)
{
    //sun light
    float c = max(-dot(lightDir_and_ambient.xyz, norm), 0);

    //ambient light
    c += lightDir_and_ambient.w;

    //clamp
    return min(max(c, 0), 1);
}

