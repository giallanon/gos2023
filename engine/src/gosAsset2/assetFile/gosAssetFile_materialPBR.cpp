#include "gosAssetFile_materialPBR.h"
#include "../gos/gosBufferWriter.h"
#include "gos.h"


using namespace gos;
using namespace gos::asset2;

//***************************************
static u32 AssetFile_materialPBR__get_serialize_size ()
{
	return 	sizeof(u32) //magic	
			+4 * sizeof(f32)	//diffuse_col
			+ sizeof(f32)		//metallic_factor
			;
}

//***************************************
u32 AssetFile_materialPBR::serialize (const MaterialPBR &a, u8 *buffer, u32 sizeof_buffer)
{
	const u32 byteNeeded = AssetFile_materialPBR__get_serialize_size();
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

	assert (ct == byteNeeded);
	return byteNeeded;
}

//***************************************
u32 AssetFile_materialPBR::deserialize (const u8 *buffer, u32 sizeof_buffer, MaterialPBR *out)
{
	assert (NULL != out);
	out->reset();

	const u32 byteNeeded = AssetFile_materialPBR__get_serialize_size();
	if (sizeof_buffer < byteNeeded)
		return 0;

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

	assert (ct == byteNeeded);
	assert (sizeof_buffer >= ct);
	return byteNeeded;	
}

//***************************************
void AssetFile_materialPBR::priv_free()
{
}

//***************************************
void AssetFile_materialPBR::begin ()
{
	mat.reset();
}

//***************************************
void AssetFile_materialPBR::set_from_materialPBR (const MaterialPBR &m)
{
	memcpy (&mat, &m, sizeof(MaterialPBR));
}

//***************************************
void AssetFile_materialPBR::set_diffuse_color_HDR_RGBA (f32 r, f32 g, f32 b, f32 a)
{
	mat.diffuse_col_RGBA_HDR[0] = r;
	mat.diffuse_col_RGBA_HDR[1] = g;
	mat.diffuse_col_RGBA_HDR[2] = b;
	mat.diffuse_col_RGBA_HDR[3] = a;
}

//***************************************
void AssetFile_materialPBR::set_metallic_factor_01 (f32 f)
{
	mat.metallic_factor = f;
}

//***************************************
void AssetFile_materialPBR::end()
{
}

//***************************************
bool AssetFile_materialPBR::save (const char *filenameDST)
{
    u8 stackBuffer[1024];

	const u32 n = serialize (mat, stackBuffer, sizeof(stackBuffer));
	if (0 == n)
	{
		DBGBREAK;
		return false;
	}
	return fs::fileSaveBuffer (filenameDST, stackBuffer, n);
}

