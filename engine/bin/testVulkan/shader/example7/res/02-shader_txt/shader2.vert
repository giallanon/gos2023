#version 450

//obj data
layout(push_constant) uniform sPushData
{
    layout(offset = 0) vec2    screenWH;    //dimensione della finestra
} pushdata;


//Input
layout(location = 0) in ivec2 in_screenXY;     //posizione del pixel in screen coordinate da (0,0) a (window_width -1, window_height -1)
layout(location = 2) in vec2  in_tutv;

//output
layout(location = 0) out vec2 out_tutv;


void main() 
{
    //pixel in alto a sx (-1,-1, 0).
    //pixel in basso a dx (1,1, 0)

    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space da passare al FragmentShader
	gl_Position.xy = ( (in_screenXY / pushdata.screenWH) * 2.0f ) - 1.0f;
    gl_Position.z = 0;
    gl_Position.w = 1;

    out_tutv = in_tutv;
}
