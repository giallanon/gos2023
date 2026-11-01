#version 450

//////////////////// descr-set 0
struct Particle
{
	uint value;
};

layout(set = 0, binding = 0) readonly buffer SBO_0_1
{
   Particle p[];
} particle_in;

layout(set = 0, binding = 1) buffer SBO_0_2
{
   Particle p[];
} particle_out;

//////////////////// push constant
layout(push_constant) uniform PushConstantData
{
    uint deltaTime;
} pc;



layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
void main()
{
	// gl_GlobalInvocationID is equal to gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID.
    const uint index = gl_GlobalInvocationID.x;

    const uint val = particle_in.p[index].value;
    particle_out.p[index].value = pc.deltaTime  + val * val;
}
