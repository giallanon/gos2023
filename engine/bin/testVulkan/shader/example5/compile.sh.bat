glslc -fshader-stage=vert --target-env=vulkan1.2 shader.vert.shader -O -o shader.vert.spv
glslc -fshader-stage=frag --target-env=vulkan1.2 shader.frag.shader -O -o shader.frag.spv


glslc -fshader-stage=vert --target-env=vulkan1.2 lineShader.vert.shader -O -o lineShader.vert.spv
glslc -fshader-stage=frag --target-env=vulkan1.2 lineShader.frag.shader -O -o lineShader.frag.spv

