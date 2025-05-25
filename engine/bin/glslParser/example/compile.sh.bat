glslc -fshader-stage=vert --target-env=vulkan1.3 lineRenderer.vert.shader -g -O -o lineRenderer.vert.spv
glslc -fshader-stage=frag --target-env=vulkan1.3 lineRenderer.frag.shader -g -O -o lineRenderer.frag.spv

glslc -fshader-stage=vert --target-env=vulkan1.3 phong.vert.shader -g -O -o phong.vert.spv
glslc -fshader-stage=frag --target-env=vulkan1.3 phong.frag.shader -g -O -o phong.frag.spv
