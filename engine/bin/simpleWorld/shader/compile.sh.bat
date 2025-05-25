glslc -fshader-stage=vert --target-env=vulkan1.3 phong.vert.shader -O -o compiled/phong.vert.spv
glslc -fshader-stage=frag --target-env=vulkan1.3 phong.frag.shader -O -o compiled/phong.frag.spv

glslc -fshader-stage=vert --target-env=vulkan1.3 PIPE_stage_clear.vert.shader -O -o compiled/PIPE_stage_clear.vert.spv

glslc -fshader-stage=vert --target-env=vulkan1.3 mapRenderer.vert.shader -O -o compiled/mapRenderer.vert.spv
glslc -fshader-stage=frag --target-env=vulkan1.3 mapRenderer.frag.shader -O -o compiled/mapRenderer.frag.spv

glslc -fshader-stage=vert --target-env=vulkan1.3 lineRenderer.vert.shader -O -o compiled/lineRenderer.vert.spv
glslc -fshader-stage=frag --target-env=vulkan1.3 lineRenderer.frag.shader -O -o compiled/lineRenderer.frag.spv
