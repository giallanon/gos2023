#ifndef _gosShapePrefabs_h_
#define _gosShapePrefabs_h_
#include "gosShape.h"

namespace gos
{ 
	namespace shape
	{
		//buildCube24()
		// Crea una cubo con 4 vtx separati per ogni faccia (24 vtx in totale) e 6 idx per faccia (36idx in totale)
		// In base a come e' statp definito [writer], filla "pos", "norm" e "texCoord"
		// Se [writer] == NULL, filla [out_info] con il num di vtx/idx necessari
		bool 	buildCube24 (const vec3f &center, const vec3f &size, VtxWriter *writer, Info *out_info);

		//buildSphere()
		//Crea la circonferenza di base attorno a [center] e poi [numHalfStack] circonferenze verso l'altro e altrettante verso il basso
		// Se [writer] == NULL, filla [out_info] con il num di vtx/idx necessari
		bool 	buildSphere (const vec3f &center, const vec3f &radius, u32 numPointPerCirconferenza, u32 numHalfStack, VtxWriter *writer, Info *out_info);
		
		//buildCylinder
		//La base e' centrata in [center] e sale verso l'alto. Crea [numStack] circonferenze (compresa quella di base)
		// Se [writer] == NULL, filla [out_info] con il num di vtx/idx necessari
		bool 	buildCylinder (const vec3f &center, f32 radius, f32 height, u32 numPointPerCirconferenza, u32 numStack, bool bCloseTop, bool bCloseBottom, VtxWriter *writer, Info *out_info);

	} //namespace shape
 } //namespace gos

#endif //_gosShapePrefabs_h_