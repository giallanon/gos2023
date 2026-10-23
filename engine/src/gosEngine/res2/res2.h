#ifndef _gosEngineRes2_res2_h_
#define _gosEngineRes2_res2_h_
#include "MemManager.h"
#include "res2_res.h"


namespace gos
{
	namespace res2
	{
		const char* 	enumToString (res2::eLoadMode s);
		const char* 	enumToString (res2::eStatus s);
		const char* 	enumToString (res2::eResType s);

		void 	setup (gos::Engine *eng, gos::Allocator *allocator, gos::Logger *logger);
		void	unsetup();
		
		bool	is_ready (const ResDescr *res);
		void	change_state (ResDescr *res, res2::eStatus new_status, const char *reason = "cng state");
		void 	inc_refCount (ResDescr *res);
		void 	add_child (ResDescr *padre, ResDescr *figlio);
		void 	remove_child (ResDescr *padre, ResDescr *figlio);

		bool	release (ResDescr *res);
		void	destroy (ResDescr *res);

	} //namespace res2
} //namespace gos

#endif //_gosEngineRes2_res2_h_