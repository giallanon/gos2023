#version 450

//input
layout(location = 0) in vec3 in_colorRGB;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_lightDir;

//output
layout(location = 0) out vec4 out_colorRGBA;
layout(location = 1) out vec4 out_normal;

void main()
{
    vec3 norm = normalize(in_normal);
    float c = max(-dot(in_lightDir.xyz, norm), 0);
    c += 0.1f;
    c = min(max(c, 0), 1);

    out_colorRGBA = vec4(in_colorRGB * c, 1);
    out_normal = vec4(norm.xyz, 1);
}
