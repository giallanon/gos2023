#ifndef _MaterialList_h_
#define _MaterialList_h_
#include "gosGPU.h"
#include "../gos/gosHandle.h"


template <class MAT_HANDLE, class MAT_DATA>
class MaterialList
{
public:
	MaterialList()
	{}

	~MaterialList()
	{
		list.unsetup();
	}

	void	setup (gos::Allocator *allocator, const MAT_DATA &defaultMaterialIN)
	{
		defaultMaterial = defaultMaterialIN;
		list.setup (allocator);
	}

	bool	add (const MAT_DATA &matData, MAT_HANDLE *out_handle)
	{
		MAT_DATA *s = list.reserve (out_handle);
		if (NULL == s)
			return false;
		*s = matData;
		return true;
	}

	const MAT_DATA*	get (const MAT_HANDLE &handle) const
	{
		MAT_DATA *ret = NULL;
		if (list.fromHandleToPointer (handle, &ret))
			return ret;
		return &defaultMaterial;
	}

private:
	MAT_DATA								defaultMaterial;
	gos::HandleList<MAT_HANDLE,MAT_DATA>	list;
};



#endif //#define _MaterialList_h_

