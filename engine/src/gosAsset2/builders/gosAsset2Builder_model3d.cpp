#include "gosAsset2Builder_model3d.h"
#include "gos.h"
#include "../gos/gosString.h"
#include "../gos/gosUtils.h"
#include "../gosAsset2Builder.h"
#include "../gos/gosBufferWriter.h"
#include "../gos/gosDataBlob.h"
#include "../assetFile//gosAssetFile_model3D.h"

using namespace gos;
using namespace gos::asset2;


//************************************
Builder_model3d::Builder_model3d () : BuilderInterface (eAssetType::model3d)
{
	which_one = 2;
}

//************************************
Builder_model3d::~Builder_model3d()
{
}

//************************************
bool Builder_model3d::build_begin (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFileIN, const gos::IniFileSection *secIN)
{
	//per capire se siamo nel caso di sintassi 1 o 2, cerco il parametro "import"
	char s[1024];
	if (secIN->get ("import", s, sizeof(s)))
	{
		which_one = 1;
		builder1.setLogger (this->logger);
	}
	else
	{
		which_one = 2;
		builder2.setLogger (this->logger);
	}

	switch (which_one)
	{
	default:	return builder1.build_begin (ctx, listof_UID_of_known_ini_file, absFilename, uid_of_iniFileIN, secIN);
	case 2:		return builder2.build_begin (ctx, listof_UID_of_known_ini_file, absFilename, uid_of_iniFileIN, secIN);
	}
}

//************************************
bool Builder_model3d::build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result)
{
	switch (which_one)
	{
	default:	return builder1.build_exe (ctx, doCreateAnAssetFile, out_bCallMeAgain, out_result);
	case 2:		return builder2.build_exe (ctx, doCreateAnAssetFile, out_bCallMeAgain, out_result);
	}
}

//************************************
void Builder_model3d::build_end()
{
	switch (which_one)
	{
	default:	builder1.build_end(); return;
	case 2:		builder2.build_end(); return;
	}
}
