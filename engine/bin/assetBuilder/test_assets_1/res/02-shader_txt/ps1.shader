#version 450

//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_colorRGBA;
layout(location = 1) out vec4 out_normal;

void main()
{
    out_colorRGBA = vec4(in_texCoord, 1, 1);
    out_normal = vec4(in_normal.xyz, 1);
}
