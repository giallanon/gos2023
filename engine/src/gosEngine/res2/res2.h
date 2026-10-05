#ifndef _gosEngineRes2_res2_h_
#define _gosEngineRes2_res2_h_
#include "MemManager.h"
#include "res2_res.h"
#include "res2_asset.h"


namespace gos
{
	namespace res2
	{
		const char* 	enumToString (res2::eLoadMode s);
		const char* 	enumToString (res2::eStatus s);
		const char* 	enumToString (res2::eResType s);
		const char* 	enumToString (res2::eAssetType s);

		void	debug__set_logger (gos::Logger *logger);
		
		bool	is_ready (const ResDescr *res);
		void	change_state (ResDescr *res, res2::eStatus new_status, const char *reason = "cng state");
		bool	release (ResDescr *res);
		void 	inc_refCount (ResDescr *res);
		

		bool	is_ready (const AssetDescr *asset);
		void	change_state (AssetDescr *asset, res2::eStatus new_status, const char *reason = "cng state");
		bool	release (AssetDescr *asset);
		void 	inc_refCount (AssetDescr *res);
		




	} //namespace res2
} //namespace gos

#endif //_gosEngineRes2_res2_h_