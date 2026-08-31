HOST_VISIBLE, HOST_COHERENT, HOST_CACHED


### HOST_COHERENT
Significa che non c'è bisogno di utilizzare
	vkFlushMappedMemoryRanges()		=> per sincronizzare la memoria CPU con la memoria GPU  (cpu write, gpu read)
	vkInvalidateMappedMemoryRanges	=> per sincronizzare la memoria GPU con la memoria CPU  (gpu write, cpu read)


### HOST_CACHED
Significa che CPU mantiene una cache sua interna per sincronizzare in maniera ottimale la lettura da parte di CPU
di memoria scritta da GPU.

Da usarsi quando si vogliono leggere in CPU le cose scritte da GPU