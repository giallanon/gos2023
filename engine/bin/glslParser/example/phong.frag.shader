#version 450
#include "phong.include.shader"


//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_colorRGBA;
//layout(location = 1) out vec4 out_color2RGBA;




void main() 
{
    const float c = PIPE_calcLight_01 (in_normal) * objInstance.width;


    //const uint ii = nonuniformEXT(material.textureIndex);
    const vec3 texCol = PIPE_sample2D_bilinear (material.textureIndex, in_texCoord).rgb
                        +texture (sampler2D(unaSolaTexture, unSoloSampler), in_texCoord).rgb;

    out_colorRGBA = vec4(material.color.rgb * texCol * c, 1);

    out_colorRGBA.xyz += objInstance.arrayDiStruct[2].v3;
    out_colorRGBA.x += objInstance.unVec2.y + objInstance.unArrayDiVec2Di6Elem[2].x;
    out_colorRGBA.x += sbbo2_2.m3[2].z;
    out_colorRGBA.x += scene.lightDir.x + objInstance.es1.fl;





//	out_color2RGBA = vec4(1,0,0,0);
}
