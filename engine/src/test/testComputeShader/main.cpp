#include "gos.h"
#include "gosGPU.h"

using namespace gos;

static constexpr u32 PARTICLE__NUM_MAX = 1024;

struct Particle
{
	u32 	value;
};

struct Data
{
	GPUPipelineHandle 		handle_pipe;
	GPUDescrPoolHandle		handle_descrPool;
	GPUStorageBufferHandle	handle_sbo_in;
	GPUStorageBufferHandle	handle_sbo_out;
	GPUDescrSetInstanceHandle   handle_descrSet0_instance;
};

//******************************** 
bool split_original_image()
{
	//checker_v1.gosimage e' la texture creata in precedenza con il metodo
	//del vtx e pxl shader.
	//La carico e salvo le sue mip-map in file separati
	gos::Allocator *allocator = gos::getScrapAllocator();


	char s[1024];
	sprintf_s (s, sizeof(s), "@w/checker_v1.gosimage");

	gos::Image image;
	if (!image::load (allocator, s, &image))
	{
		logger::err ("split_original_image::load() => file not found %s\n", s);
		return false;
	}
	
	const image::sImageHeader *header = image::get_info (image);
	for (u8 i=0; i<header->numTexture; i++)
	{
		const image::sTextureHeader *tex_info = image::get_texture_info (image, i);
		for (u8 mip=0; mip<tex_info->numMipMap; mip++)
		{
			image::sTextureData tex_data;
			image::get_texture_data (image, i, mip, &tex_data);

			sprintf_s (s, sizeof(s), "@w/checker_v1__%02d__%02d.buffer", i, mip);
			gos::File f;
			fs::fileOpenForW (&f, s);
			fs::fileWrite (f, tex_data.textureData, tex_data.compressed_sizeInByteOfTextureData);
			fs::fileClose(f);
		}
	}



	image::free (allocator, image);
	return true;
}


//******************************** 
bool compile_shader(gos::GPU *gpu)
{
	char s[128];
	char src[1024];
	char dst[1024];
	sprintf_s (s, sizeof(s), "@w/shader1.comp.shader");
	fs::resolvePath(s, src, sizeof(src));
	sprintf_s (s, sizeof(s), "@w/shader1.comp.shader.spv");
	fs::resolvePath(s, dst, sizeof(dst));

	//creo la versione ottimizzata e la versione con le debug-info. Quest'ultima
	//serve per esempio alle pipeline_def per recuperare i nomi e il formato dei descrittori
	if (!GPU::shader_compile (src, "comp", NULL, dst, false))
	{
		logger::err ("compile_shader() => error compiling shader %s\n", src);
		return false;
	}

	strcat_s (dst, sizeof(dst), "d");
	if (!GPU::shader_compile (src, "comp", NULL, dst, true))
	{
		logger::err ("compile_shader() => error compiling shader %s\n", src);
		return false;
	}

	return true;
}

//******************************** 
bool create_pipe (gos::GPU *gpu, Data *data)
{
	char s[1024];
	sprintf_s (s, sizeof(s), "@w/shader1.comp.shader.spv");
	GPUShaderHandle handle_shader;
	if (!gpu->compshader_createFromFile (s, "main", &handle_shader))
	{
		logger::err ("create_pipe() => error loading shader %s\n", s);
		return false;
	}
	
	
	gpu::Pipeline_def 	def;
	def.reset()
		.shader_add (handle_shader)
		.pushConst_add (0, sizeof(f32), eShaderType::compute)
        .descriptorset_add()
            .add (0, eGPUDescriptrorType::STORAGE_BUFFER, 1, eGPUDescriptrorUsage::compute_shader)
			.add (1, eGPUDescriptrorType::STORAGE_BUFFER, 1, eGPUDescriptrorUsage::compute_shader)
            .endDescriptorSet();

    if (!gpu->pipeline_createNew_compute (def, &data->handle_pipe))
    {
        gos::logger::err ("create_pipe() => can't create pipeline\n");
        return false;
    };    


	gpu->deleteResource (handle_shader);



	//creo un descriptor pool
	gpu->descrPool_createNew (&data->handle_descrPool)
		.setMaxNumDescriptorSet(4)
		.addPool_uniformBuffer()
		.addPool_storageBuffer()
		.end();
	if (data->handle_descrPool.isInvalid())
	{
		gos::logger::err ("can't create decripto pool\n");
		return false;
	}

	const u32 size = PARTICLE__NUM_MAX * sizeof(Particle);
	gpu->storageBuffer_create (size, eMemAccessMode::shared_cpuW, &data->handle_sbo_in);
	gpu->storageBuffer_create (size, eMemAccessMode::shared_cpuR, &data->handle_sbo_out);

	//alloco una istanza del descriptorSet
	if (!gpu->descrSetInstance_create (data->handle_descrPool, data->handle_pipe, 0, &data->handle_descrSet0_instance))
	{
		gos::logger::err ("an't create descriptorSet instance\n");
		return false;
	}	
	
	return true;
}


//******************************** 
void compute (GPUCmdBufferHandle &handle_cmdBuffer, gos::GPU *gpu, Data *data)
{
	u32 pp[PARTICLE__NUM_MAX];
	for (u32 i=0; i<PARTICLE__NUM_MAX; i++)
		pp[i] = i;

	gpu::MappedBufW mb;
	gpu->begin_write (data->handle_sbo_in, &mb);
		mb.write (pp, sizeof(pp), 0);
		mb.end();



    gos::gpu::DescrSetInstanceWriter descrWriter;
    descrWriter.begin (gpu, data->handle_descrSet0_instance)
        .bindStorageBuffer (0, data->handle_sbo_in)
		.bindStorageBuffer (1, data->handle_sbo_out)
        .end();
		
	
	u32 deltaTime = 100;

	gos::gpu::CmdBufferWriter2 cw;
	cw.begin (gpu, handle_cmdBuffer)
		.compute_begin()
			.bindPipeline (data->handle_pipe)
			.bindDescriptorSet (data->handle_descrSet0_instance, 0)
			.pushConstant (0, &deltaTime, sizeof(deltaTime))
			.dispatch(PARTICLE__NUM_MAX/256, 1, 1)
		.compute_end()
	.end();	
		
}

//******************************** 
int main (int argc, char *argv[])
{
    gos::sGOSInit init;
    init.memory__set_default_for_NON_games();
    init.set_log_mode (gos::sGOSInit::eLogMode::only_console);
    if (!gos::init (init, "testComputeShader"))
	{
		printf ("Error during GOS init\n");
        return -1;
	}

	gos::GPU gpu;
	if (!gpu.init (GOSWinHandle::INVALID(), false))
	{
		logger::err ("Can't init GPU\n");
		return -2;
	}

	//split_original_image();
	compile_shader(&gpu);

	Data data;
	if (create_pipe(&gpu, &data))
	{
		GPUCmdBufferHandle  handle_cmdBuffer;
		gpu.cmdBuffer_create (eGPUQueueFamily::compute, &handle_cmdBuffer);

		gpu::ComputeJob job;
		job.setup (&gpu);
		
		compute(handle_cmdBuffer, &gpu, &data);
		job.submit(handle_cmdBuffer);
		while (!job.hasFinished())
		{
			gos::sleep_msec(1);
		}

		const u32 *p = reinterpret_cast<const u32*>( gpu.readback(data.handle_sbo_out) );
		job.unsetup();

		gpu.deleteResource (handle_cmdBuffer);
	}

	gpu.deleteResource (data.handle_sbo_in);
	gpu.deleteResource (data.handle_sbo_out);
	gpu.deleteResource (data.handle_descrSet0_instance);
	gpu.deleteResource (data.handle_pipe);
	gpu.deleteResource (data.handle_descrPool);
	gpu.deinit();
    
    

#if defined(GOS_PLATFORM__WINDOWS) && defined(_DEBUG)
    printf ("\n\nPress a key to terminate\n");
    _getch();
#endif
    
    gos::deinit();
    return 0;
}