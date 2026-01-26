#include "gosModel.h"
#include "../gos/gosMagicUID.h"
#include "../gosEngine.h"

using namespace gos;

//************************** 
bool model::isValid (const Model &m)
{
	if (NULL == m.allocator)
		return false;
	const u32 magic = utils::bufferReadU32 (m.blob, 0);
	return (GOS_MAGIC__ENGINE_MODEL == magic);	
}

//************************** 
void model::free (Model &m)
{
	if (NULL == m.allocator)
		return;
	GOSFREE(m.allocator, m.blob);
	m.reset();
}