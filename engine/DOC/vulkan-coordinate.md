### gl_Position
E' espressa in clip-space coordinate, che sono coordinate omogeneo (x,y,z,w)
Il cubo rappresentato da queste coordinate è un cubo (-w, -w, 0) - (w, w, 0)


(-w,-w,0)              (0,-w,0)
                          |   
                          |  /(0,0,w)
                          | /
                          |/
(-w,0,0)---------------(0,0,0)-------------->(w,0,0)
                          |
                          |
                          |
                          V
                       (0,w,0)                (w,w,0)


### NDC  (Normalized device coord)
NDC = clip-space-coord / w
viene fatto automaticamente da fragshader e riporta le coordinate omogenee di clip-space
in un cubo (-1,-1,0) - (1,1,1)


(-1,-1,0)              (0,-1,0)              (1,-1,0)
                          |   
                          |  /(0,0,1)
                          | /
                          |/
(-1,0,0)---------------(0,0,0)-------------->(1,0,0)
                          |
                          |
                          |
                          V
(-1,1,0)               (0,1,0)               (1,1,0)

la coordinata z va da 0 a 1, con 0=near e 1=far

### Screen space (deto anche FrameBuffer space)
dove:
	w = width del frame buffer (es: 1920)
	h = height del frame buffer (es: 1080)
	
La coordinata z va da 0 a 1

(0,0)                           (w,0) 
   ------------------------------->
   |
   |
   |
   |
   |
   |
   |
   V
(0,h)                           (w,h)



### VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
Uniform buffers are great for small, read only data
With uniform buffers (UBO), only a small amount can be accessed in the shader (vendor dependant, 16 kilobytes guaranteed minimum) and the memory will be read-only


### VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
Storage buffers are usually slightly slower than uniform buffers, but they can be much, much bigger. If you want to stuff your entire scene into one buffer, you have to use them
Storage buffers (SSBO) are fully generic read-write buffers with very high size. Spec minimum size is 128 megabytes

### DESCRIPTOR
When creating the descriptors, its also possible to have them as DYNAMIC BUFFER. If you use that, you can control the offset the buffer is bound to when writing the commands. This lets you use 1 descriptor set for multiple objects draws, by storing the uniform data for multiple objects into a big buffer, and then binding that descriptor at different offsets within that. It works well for uniform buffers, but for storage buffers its better to go with device-address.


VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT: 
Once you bind a descriptor set and use it in a vkCmdDraw() function, you can no longer modify it unless you specify the
VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT flag

VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
it is possible to use descriptor sets, and bind them in command buffers, and update it right before submitting the command buffer. This is mostly a niche use case, and not commonly used. You can only update a descriptor set before it’s bound for the first time, unless you use that flag, in which case you can only update it before you submit the command buffer into a queue. When a descriptor set is being used, it’s immutable, and trying to update it will cause errors. The validation layers catch that. To be able to update the descriptor sets again, you need to wait until the command has finished executing.
