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

//***************************************
u8	skeleton::get_bone_num (const Skeleton &sk)
{
	return sk.blob[8];
}

//***************************************
const Bone* skeleton::get_bone_list (const Skeleton &sk)
{
	return reinterpret_cast <const Bone*>(&sk.blob[12]);	
}
Bone* skeleton__get_bone_list__no_const (const Skeleton &sk)
{
	return reinterpret_cast <Bone*>(&sk.blob[12]);	
}


//***************************************
void skeleton::clone (const Skeleton &sk, gos::Allocator *allocatorIN, Skeleton *out)
{
	assert (NULL != out);
	out->allocator = allocatorIN;
	out->blob = NULL;
	
	const u32 size = get_blob_size(sk);
	out->blob = GOSALLOCT(u8*, allocatorIN, size);
	memcpy (out->blob, sk.blob, size);
}

//***************************************
void skeleton__resolve_ric (const gos::Bone *sk_boneList, u32 boneIndex, const mat4x4f &parent_matW, gos::Bone *out_boneList)
{
    Bone *bone = &out_boneList[boneIndex];
    bone->matrix = parent_matW * sk_boneList[boneIndex].matrix;
    
    u32 childrenIndex = bone->firstChildIndex;
    while (0xFF != childrenIndex)
    {
        skeleton__resolve_ric (sk_boneList, childrenIndex, bone->matrix, out_boneList);
        childrenIndex = out_boneList[childrenIndex].sigblinIndex;
    }
}
void skeleton::resolve (const Skeleton &sk, const mat4x4f &matW, Skeleton *out)
{
	skeleton__resolve_ric (get_bone_list(sk), 0, matW, skeleton__get_bone_list__no_const(*out));
}

//***************************************
void skeleton::translate (Skeleton &sk, const vec3f &s)
{
	Bone *boneList = skeleton__get_bone_list__no_const (sk);

	//mi basta traslare root
	boneList[0].matrix(0,3) = boneList[0].matrix(0,3) + s.x;
	boneList[0].matrix(1,3) = boneList[0].matrix(1,3) + s.y;
	boneList[0].matrix(2,3) = boneList[0].matrix(2,3) + s.z;
}

//***************************************
void skeleton::scale (Skeleton &sk, const vec3f &s)
{
	Bone *boneList = skeleton__get_bone_list__no_const (sk);
	
	const u8 n = get_bone_num(sk);
	for (u8 i=0; i<n; i++)
	{
		//scalo solo il posizione della matrice
		boneList[i].matrix(0,3) = boneList[i].matrix(0,3) * s.x;
		boneList[i].matrix(1,3) = boneList[i].matrix(1,3) * s.y;
		boneList[i].matrix(2,3) = boneList[i].matrix(2,3) * s.z;
	}
}