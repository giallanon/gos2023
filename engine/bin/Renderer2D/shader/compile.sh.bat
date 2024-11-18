glslc -fshader-stage=vert --target-env=vulkan1.2 renderer2D.vert.shader -O -o renderer2D.vert.spv
glslc -fshader-stage=frag --target-env=vulkan1.2 renderer2D.frag.shader -O -o renderer2D.frag.spv
