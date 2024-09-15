#version 450
#extension GL_EXT_nonuniform_qualifier : require

//scene
layout(set = 0, binding = 0) uniform UBO_0_0
{
    mat4 camVP;
    vec4 lightDir;
} scene;

layout(set = 1, binding = 0) uniform sampler2D sampler_diffuse[];

layout(set = 2, binding = 0) uniform UBO_2_0
{
    vec3    color;
    uint    textureIndex;
} material;

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

    uint ii = material.textureIndex;
    out_colorRGBA = vec4((material.color * texture (sampler_diffuse[nonuniformEXT(ii)], in_texCoord).rgb) * c, 1);
}
