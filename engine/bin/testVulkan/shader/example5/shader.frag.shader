#version 450

//input
layout(location = 0) in vec3 in_colorRGB;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_lightDir;

//output
layout(location = 0) out vec4 out_colorRGBA;


void main() 
{
    float c = max(-dot(in_lightDir.xyz, in_normal), 0);
    c += 0.1f;
    c = min(max(c, 0), 1);
    out_colorRGBA = vec4(in_colorRGB * c, 1);
}
