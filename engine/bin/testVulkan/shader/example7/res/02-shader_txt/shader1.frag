#version 450

layout (set=0, binding=0) uniform sampler2D texSampler;

//input
layout(location = 0) in vec2 in_tutv;

//output
layout(location = 0) out vec4 out_colorRGBA;

void main() 
{
    out_colorRGBA = vec4 (texture(texSampler, in_tutv).rgb, 1);
}
