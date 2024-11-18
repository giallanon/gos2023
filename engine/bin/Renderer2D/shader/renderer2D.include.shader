layout(set = 0, binding = 0) uniform sampler    samplerList[];
layout(set = 0, binding = 1) uniform texture2D  textureList[];

//scene
layout(set = 1, binding = 0) uniform LAYOUT_SCENE_DATA
{
    mat4 camVP;
} scene;


