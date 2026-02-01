#include "gosSkeleton.h"
#include "../gos/gosMagicUID.h"

using namespace gos;


//***************************************
bool skeleton::isValid (const Skeleton &sk)
{
	if (NULL == sk.allocator)
		return false;
	const u32 magic = utils::bufferReadU32 (sk.blob);
	return (GOS_MAGIC__SKELETON == magic);	
}

//***************************************
void skeleton::free (Skeleton &sk)
{
	if (NULL == sk.allocator)
		return;
	GOSFREE(sk.allocator, sk.blob);
	sk.reset();
}

//***************************************
u32  skeleton::get_blob_size (const Skeleton &sk)
{
	if (!isValid(sk))
		return 0;
	return utils::bufferReadU32 (&sk.blob[4]);
}


//***************************************
u32  skeleton::serialize (const Skeleton &sk, u8 *buffer, u32 sizeof_buffer)
{
	const u32 magic = utils::bufferReadU32 (sk.blob);
	if (GOS_MAGIC__SKELETON != magic)
	{
		DBGBREAK;
		return 0;
	}

	const u32 total_size_of_blob = utils::bufferReadU32 (&sk.blob[4]);
	if (NULL == buffer)
		return total_size_of_blob;

	if (sizeof_buffer < total_size_of_blob)
	{
		DBGBREAK;
		return 0;
	}

	memcpy (buffer, sk.blob, total_size_of_blob);
	return total_size_of_blob;
}

//***************************************
u32 skeleton::deserialize (const u8 *buffer, u32 sizeof_buffer, gos::Allocator *allocator, Skeleton *out)
{
	assert (NULL != out);
	if (sizeof_buffer < 12)
	{
		DBGBREAK;
		return 0;
	}

	u32 ct = 0;

	//magic
	const u32 magic = gos::utils::bufferReadU32 (&buffer[ct]);
	ct += 4;

	if (!magic::signatureMatch (magic, GOS_MAGIC__SKELETON) || !magic::versionMatch (magic, GOS_MAGIC__SKELETON))
	{
		DBGBREAK;
		return 0;
	}

	const u32 total_size_of_blob = gos::utils::bufferReadU32 (&buffer[ct]);


	if (sizeof_buffer < total_size_of_blob)
	{
		DBGBREAK;
		return 0;
	}

	out->allocator = allocator;
	out->blob = GOSALLOCT(u8*, allocator, total_size_of_blob);
	memcpy (out->blob, buffer, total_size_of_blob);
	
	return total_size_of_blob;
}

//***************************************
static void skeleton_debug__print_matrix (gos::UTF8String &out, const mat4x4f &m)
{
	for (u32 row=0; row<4; row++)
	{
		out << "[" << STRFMT("%.2f %.2f %.2f %.2f", m(row,0), m(row,1), m(row,2), m(row,3)) <<"] ";
	}
}
static void skeleton_debug__print_rec (gos::UTF8String &out, u32 indent, const skeleton::Reader &reader, u32 bone_index)
{
	const Bone *bone = reader.bone_get_by_index(bone_index);

	out.fillRowUntilColumn (indent, ' ');
    out << "name: " << reader.name_get_by_index(bone_index) << "   ";
	skeleton_debug__print_matrix (out, bone->matrix);
	out << "\n";
    

    u8 index = bone->firstChildIndex;
    while (0xFF != index)
    {
        bone =  reader.bone_get_by_index(index);
        skeleton_debug__print_rec (out, indent+4, reader, index);
        index = bone->sigblinIndex;
    }
}
void skeleton::debug__print (const Skeleton &sk, gos::UTF8String &out)
{
	Reader reader;
	reader.setup(&sk);

    out << "Num bones: "<< STRFMT("%d", reader.bone_get_num()) << "\n";
    skeleton_debug__print_rec (out, 0, reader, 0);
}
