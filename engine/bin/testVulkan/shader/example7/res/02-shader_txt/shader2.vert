#version 450

vec2 tutvList[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),

    vec2(1.0, 1.0),
    vec2(0.0, 1.0),
    vec2(0.0, 0.0)
);

//obj data
layout(push_constant) uniform sPushData
{
    vec2    screenWH;    //dimensione della finestra in pixel
    vec2    quadSize;    //dimensione della quad in pixel
} pushdata;


//output
layout(location = 0) out vec2 out_tutv;


void main()
{
    //pixel in alto a sx (-1,-1, 0).
    //pixel in basso a dx (1,1, 0)
    out_tutv = tutvList[gl_VertexIndex].xy;

    vec2 quadWH = out_tutv.xy * pushdata.quadSize.xy;

    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space da passare al FragmentShader
	gl_Position.x = ( (quadWH.x / pushdata.screenWH.x) * 2.0f ) - 1.0f;
    gl_Position.y = ( (quadWH.y / pushdata.screenWH.y) * 2.0f ) - 1.0f;
    gl_Position.z = 0;
    gl_Position.w = 1;
}
