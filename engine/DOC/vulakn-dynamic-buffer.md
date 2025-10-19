### dynamic buffer
A dynamic uniform buffer (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) is almost identical to a uniform buffer, and differs only in how the offset into the buffer is specified. The base offset calculated by the VkDescriptorBufferInfo when initially updating the descriptor set is added to a dynamic offset when binding the descriptor set.

Quando si binda un buffer STATICO ad un descrittore, lo si binda sempre a partire
da offset = 0.
I buffer dinamici invece, sono bindati a partire da un offset scelto dall'utente.
Quando lo shader accede all'elemento [0], in realtà sta accedendo a [offset + 0].

Attenzione che la dimensione minima di uno "slot" in un buffer è governata da "minUniformBufferOffsetAlignment".
Ogni slot deve quindi essere grosso almeno "minUniformBufferOffsetAlignment" byte


### bindless
I descrittori devono essere creati con
    - "VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT" il che vuol dire che li posso aggiornare anche dopo che sono stati bindati ad una pipeline.
    
    - "VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT"

Nella creazione di un bindless descriptor, devo comunque indicare un numero massimo di elementi dell'array


### GOS declaration
In uno shader, un buffer dichiarato  [] viene considerato bindless.
In uno shader, un buffer il cui nome inzi con dyn_ viene considerato dynamic.

es:
    layout(set = 0, binding = 1) uniform texture2D textureList[];       //bindless, statico
    layout(set = 0, binding = 1) uniform texture2D dyn_textureList[];   //bindless, dynamic
    layout(set = 0, binding = 1) uniform texture2D textureList[8];      //statico
    layout(set = 0, binding = 1) uniform texture2D dyn_textureList[8];  //dynamic