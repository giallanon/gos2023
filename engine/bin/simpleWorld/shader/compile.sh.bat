glslc -fshader-stage=vert --target-env=vulkan1.2 phong.vert.shader -O -o phong.vert.spv
glslc -fshader-stage=frag --target-env=vulkan1.2 phong.frag.shader -O -o phong.frag.spv


glslc -fshader-stage=vert --target-env=vulkan1.2 mapRenderer.vert.shader -O -o mapRenderer.vert.spv
glslc -fshader-stage=frag --target-env=vulkan1.2 mapRenderer.frag.shader -O -o mapRenderer.frag.spv
