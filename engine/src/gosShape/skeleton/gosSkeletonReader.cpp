#include "gosSkeleton.h"

using namespace gos;
using namespace gos::skeleton;


//***********************************************
void Reader::setup (const Skeleton *skIN)
{
	sk = skIN;
}

//***********************************************
u32 Reader::bone_get_num() const
{
	return sk->blob[8];
}

//***********************************************
const Bone* Reader::bone_get_by_index (u32 i) const
{
	assert (i < bone_get_num());

	u32 ct = 12 + sizeof(Bone) * i;
	return reinterpret_cast <const Bone*>(&sk->blob[ct]);
}

//***********************************************
const Bone* Reader::bone_get_by_name (const char *s) const
{
	const u32 n = bone_get_num();
	for (u32 i = 0; i < n; i++)
	{
		if (string::utf8::areEqual(s, name_get_by_index(i), false))
			return bone_get_by_index(i);
	}

	DBGBREAK;
	return NULL;
}

//***********************************************
u32 Reader::bone_get_index_by_name (const char *s) const
{
	const u32 n = bone_get_num();
	for (u32 i = 0; i < n; i++)
	{
		if (string::utf8::areEqual(s, name_get_by_index(i), false))
			return i;
	}

	DBGBREAK;
	return u32MAX;
}

//***********************************************
const char* Reader::name_get_by_index (u32 i) const
{
	assert (i < bone_get_num());

	const u32 start_of_name_table = utils::bufferReadU16 (&sk->blob[10]);
	const u32 str_offset = utils::bufferReadU16 (&sk->blob[start_of_name_table + i * sizeof(u16)]);

	return reinterpret_cast<const char*>(&sk->blob[str_offset]);
}

