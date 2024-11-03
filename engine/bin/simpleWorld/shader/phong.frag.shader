#version 450
#include "phong.include.shader"


//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_colorRGBA;

void main() 
{
    float c = max(-dot(scene.lightDir.xyz, in_normal), 0);
    c += scene.lightDir.w;
    c = min(max(c, 0), 1);

    //const uint ii = nonuniformEXT(material.textureIndex);
    const uint ii = material.textureIndex;

    const vec3 texCol = texture (sampler2D(textureList[ii], samplerList[0]), in_texCoord).rgb;
    out_colorRGBA = vec4(material.color.rgb * texCol * c, 1);
    
    //out_colorRGBA = vec4((material.color * texture (sampler_diffuse[nonuniformEXT(ii)], in_texCoord).rgb) * c, 1);
}
