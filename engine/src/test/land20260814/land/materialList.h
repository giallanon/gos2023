#ifndef _land_materialList_h_
#define _land_materialList_h_
#include "enumAndDefine.h"
#include "dataTypes/gosColorHDR.h"

namespace land
{
	struct Material
	{
		f32		diffuse_r;
		f32		diffuse_g;
		f32		diffuse_b;
		f32		pad0;
	};

	/****************************************
	 * @brief	MaterialList
	 * 
	 */
	class MaterialList
	{
	public:
						MaterialList();
						~MaterialList()							{ }

		void 			add (u8 materialID, u32 rgb);

		u32				get_num() const							{ return num_material; }
		const Material*	get_by_id (u8 materialID) const;
		const Material*	get_list() const 						{ return list_of_material; }
	
	private:
		static constexpr u8 MATERIAL__NUM_MAX = 32;

	private:
		u8			list_of_materialID[MATERIAL__NUM_MAX];
		Material	list_of_material[MATERIAL__NUM_MAX];
		u8			num_material;
	};

} //namespace land


#endif //_land_materialList_h_

