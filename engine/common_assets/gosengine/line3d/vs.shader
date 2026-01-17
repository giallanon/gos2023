#version 450
#include "common.shader"

//Input
layout(location = 0) in vec2 in_position;

//output
layout(location = 0) out vec4 out_colorRGBA;


void main()
{
    //indici del vtx-start  vtx-end della linea in 3d
    const uint packed_idx_1_2 = indexList.idx[gl_InstanceIndex];
    const uint idx1 = 3 * (packed_idx_1_2 & 0xFF00) >> 8;
    const uint idx2 = 3 * (packed_idx_1_2 & 0x00FF);

    //vertici start-vtx e end-vtx in world pos
    const vec3 world_pos1 = vec3 (vtxList.pos[idx1], vtxList.pos[idx1+1], vtxList.pos[idx1+2]);
    const vec3 world_pos2 = vec3 (vtxList.pos[idx2], vtxList.pos[idx2+1], vtxList.pos[idx2+2]);

    //..in clip pos
    const vec4 clip_pos1 = vec4(world_pos1, 1.0) * scene.camVP;
    const vec4 clip_pos2 = vec4(world_pos2, 1.0) * scene.camVP;

    //.. in screen pos
    const vec2 screen_pos1 = ((clip_pos1.xy / clip_pos1.w) + 1) * 0.5 * scene.screen_wh.xy;
    const vec2 screen_pos2 = ((clip_pos2.xy / clip_pos2.w) + 1) * 0.5 * scene.screen_wh.xy;

    //asse x e y in screen coord del segmento da disegnare
    const vec2 ax = screen_pos2 - screen_pos1;
    const vec2 ay = normalize (vec2(-ax.y, ax.x));

    //in_position.x vale 0 se il vertice e' all'inizio del segmento, vale 1 se e' alla fine del segmento
    vec2 p0 = screen_pos1 + in_position.x * ax + pc.line_width * in_position.y * ay;

    //interpolo le componenti z e w in clip space
    const vec4 clip = mix(clip_pos1, clip_pos2, in_position.x);

    //passo al PS il vertice in clip space
    gl_Position = vec4(
        clip.w *  ( ( (p0.xy / scene.screen_wh.xy) * 2.0) - 1.0),
        clip.z,
        clip.w
    );


    out_colorRGBA = pc.color_RGBA;
}

