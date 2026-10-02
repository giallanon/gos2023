#ifndef _gosEngineRes2_h_
#define _gosEngineRes2_h_
#include "enumAndDefine.h"
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
		
		void	res_setup (ResDescr *res, asset2::UID signatureUID, ResHandle res_handle);
		bool	res_is_ready (const ResDescr *res);
		void	res_change_state (ResDescr *res, res2::eStatus new_status, const char *reason = "cng state");
		bool	res_release (ResDescr *res);
		

		void	asset_setup (AssetDescr *asset, asset2::UID assetUID, AssetHandle asset_handle);
		bool	asset_is_ready (const AssetDescr *asset);
		void	asset_change_state (AssetDescr *asset, res2::eStatus new_status, const char *reason = "cng state");
		bool	asset_release (AssetDescr *asset);
		




	} //namespace res2
} //namespace gos

#endif //_gosEngineRes2_h_