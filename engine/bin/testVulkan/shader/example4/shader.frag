#version 450

//input
layout(location = 0) in vec3 in_colorRGB;
layout(location = 1) in vec2 in_texCoord0;

//output
layout(location = 0) out vec4 out_colorRGBA;


layout (set=0, binding=1) uniform sampler2D texSampler;

void main() 
{
    out_colorRGBA = vec4 (in_colorRGB * texture(texSampler, in_texCoord0).rgb, 1);
}