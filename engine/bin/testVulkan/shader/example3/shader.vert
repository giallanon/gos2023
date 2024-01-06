#version 450

//Input
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_colorRGB;

//output
layout(location = 0) out vec3 out_colorRGB;


void main() 
{
    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space passate al FragmentShader
    gl_Position = vec4(in_position, 0.0, 1.0);
    out_colorRGB = in_colorRGB;
}
