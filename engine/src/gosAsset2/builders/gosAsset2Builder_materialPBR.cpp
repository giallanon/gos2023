#include "gosAsset2Builder_materialPBR.h"
#include "gos.h"
#include "../gos/gosString.h"
#include "../gosAsset2Builder.h"




using namespace gos;
using namespace gos::asset2;

//************************************
Builder_materialPBR::Builder_materialPBR () : BuilderInterface (eAssetType::materialPBR)
{ 
}

//************************************
bool Builder_materialPBR::priv_extractParams (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename)
{
    assert (NULL != sec);
    
    //setto i default
   	params.mat.begin();

    //parse della section
	string::utf8::StringListParser sp;
    char s[1024];
	f32	floatArray[16];



    //param:diffuse_col_RGBA_HDR      e' opzionale
    if (sec->get("diffuse_col_RGBA_HDR", s, sizeof(s)))
    {
		sp.toStart(s, ",");
		if (!sp.extract_f32Array (floatArray, 4))
        {
            logger->log(eTextColor::red, "line %d => invalid value '%s' for <diffuse_col_RGBA_HDR>\n", sec->getLineStarted(), s);
            return false;
        }
		params.mat.set_diffuse_color_HDR_RGBA (floatArray[0], floatArray[1], floatArray[2], floatArray[3]);
	}


    //param:metallic_factor      e' opzionale
    params.mat.set_metallic_factor_01 (sec->getOrDefaultAsF32("metallic_factor", 0.0f));

	params.mat.end();
    return true;
}

//************************************
bool Builder_materialPBR::build_begin (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFileIN, const gos::IniFileSection *secIN)
{
    assert (ctx.isValid());
    assert (NULL != secIN);
	uid_of_iniFile = uid_of_iniFileIN;
	sec = secIN;
    

    //parse dei params
    if (!priv_extractParams(ctx, listof_UID_of_known_ini_file, absFilename))
    {
        logger->log (eTextColor::red, "error parsing IniFileSection\n");
        return false;
    }
	

    return true;
}

//************************************
bool Builder_materialPBR::build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result)
{
	assert (NULL != out_result);
	assert (NULL != out_bCallMeAgain);
	*out_bCallMeAgain = false;
    out_result->reset();


    //setup di virtual-asset
    //All'uscita da questa fn:
    //  out_result->uid_virtual_asset       contiene l'UID di questo virtual asset, gia' inserito nel DB
    //  out_result->uid_concrete_asset      contiene l'UID dell'asset concreto a cui questo virtual-asset punta
    //  out_result->result                  vale <eBuildResult::just_built> se e' necessario creare fisicamente il concrete-asset, altrimenti vale <eBuildResult::was_already_built>
    if (!prot_setupVirtualAsset (ctx, &params, sizeof(Params), uid_of_iniFile, sec, out_result))
        return false;

   
    //a questo punto devo davvero creare il file dell'asset
    if (doCreateAnAssetFile && eBuildResult::just_built == out_result->result)
    {
        char filenameDST[1024];
        asset_manufacture_fullFilename (ctx, out_result->uid_concrete_asset, filenameDST, sizeof(filenameDST));
		return params.mat.save (filenameDST);
    }

	return true;
}



