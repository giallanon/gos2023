#include "gosAssetFile_materialPBR.h"
#include "../gos/gosBufferWriter.h"
#include "gos.h"


using namespace gos;
using namespace gos::asset2;

//***************************************
static constexpr u32 MaterialPBR__get_serialize_size()
{
	return 	sizeof(u32) //magic	
			+4 * sizeof(f32)	//diffuse_col
			+ sizeof(f32)		//metallic_factor
			+ sizeof(UID)		//diffuse texture
			+ sizeof(UID)		//normal texture
			;
}

//***************************************
u32 MaterialPBR::serialize (const MaterialPBR &a, u8 *buffer, u32 sizeof_buffer)
{
	const u32 byteNeeded = MaterialPBR__get_serialize_size();
	if (NULL == buffer)
		return byteNeeded;

	if (sizeof_buffer < byteNeeded)
		return 0;

	u32 ct = 0;

	//magic
	ct += gos::utils::bufferWriteU32 (&buffer[ct], GOS_MAGIC__ASSET_MATERIAL_PBR);

	ct += gos::utils::bufferWriteF32 (&buffer[ct], a.diffuse_col_RGBA_HDR[0]);
	ct += gos::utils::bufferWriteF32 (&buffer[ct], a.diffuse_col_RGBA_HDR[1]);
	ct += gos::utils::bufferWriteF32 (&buffer[ct], a.diffuse_col_RGBA_HDR[2]);
	ct += gos::utils::bufferWriteF32 (&buffer[ct], a.diffuse_col_RGBA_HDR[3]);

	ct += gos::utils::bufferWriteF32 (&buffer[ct], a.metallic_factor);

	ct += gos::utils::bufferWriteU64 (&buffer[ct], a.diffuse_texture_uid._uid);
	ct += gos::utils::bufferWriteU64 (&buffer[ct], a.normal_texture_uid._uid);

	assert (ct == byteNeeded);
	return byteNeeded;
}

//***************************************
u32 MaterialPBR::deserialize (const void *bufferIN, u32 sizeof_buffer, MaterialPBR *out)
{
	assert (NULL != out);
	out->reset();

	const u32 byteNeeded = MaterialPBR__get_serialize_size();
	if (sizeof_buffer < byteNeeded)
		return 0;

	const u8 *buffer = reinterpret_cast<const u8*>(bufferIN);
	u32 ct = 0;

	//magic
	const u32 magic = gos::utils::bufferReadU32 (&buffer[ct]);
	ct += 4;

	if (!magic::signatureMatch (magic, GOS_MAGIC__ASSET_MATERIAL_PBR) || !magic::versionMatch (magic, GOS_MAGIC__ASSET_MATERIAL_PBR))
	{
		DBGBREAK;
		return 0;
	}

	out->diffuse_col_RGBA_HDR[0] = gos::utils::bufferReadF32 (&buffer[ct]);	ct += 4;
	out->diffuse_col_RGBA_HDR[1] = gos::utils::bufferReadF32 (&buffer[ct]);	ct += 4;
	out->diffuse_col_RGBA_HDR[2] = gos::utils::bufferReadF32 (&buffer[ct]);	ct += 4;
	out->diffuse_col_RGBA_HDR[3] = gos::utils::bufferReadF32 (&buffer[ct]);	ct += 4;

	out->metallic_factor = gos::utils::bufferReadF32 (&buffer[ct]);	ct += 4;

	out->diffuse_texture_uid._uid = gos::utils::bufferReadU64 (&buffer[ct]);	ct += 8;
	out->normal_texture_uid._uid = gos::utils::bufferReadU64 (&buffer[ct]);	ct += 8;

	assert (ct == byteNeeded);
	assert (sizeof_buffer >= ct);
	return byteNeeded;	
}

//***************************************
void MaterialPBR::reset()
{
	diffuse_col_RGBA_HDR[0] = diffuse_col_RGBA_HDR[1] = diffuse_col_RGBA_HDR[2] = diffuse_col_RGBA_HDR[3] = 1.0f;
	metallic_factor = 0;
	diffuse_texture_uid.setInvalid();
	normal_texture_uid.setInvalid();
}

//***************************************
bool MaterialPBR::save (const char *filenameDST) const
{
    u8 stackBuffer[1024];

	const u32 n = serialize (*this, stackBuffer, sizeof(stackBuffer));
	if (0 == n)
	{
		DBGBREAK;
		return false;
	}
	return fs::fileSaveBuffer (filenameDST, stackBuffer, n);
}

