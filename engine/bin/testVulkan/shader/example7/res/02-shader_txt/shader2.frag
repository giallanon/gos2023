#version 450

//input
layout(location = 0) in vec2 in_tutv;

//output
layout(location = 0) out vec4 out_colorRGBA;

void main() 
{
    vec3 col = vec3(1,0,0);
    if (in_tutv.x < 0.01f || in_tutv.x > 0.99f || in_tutv.y < 0.01f || in_tutv.y > 0.99f)
    {
        col.r = col.g = col.b = 1.0f;
    }

    out_colorRGBA = vec4(col, 1);
}
