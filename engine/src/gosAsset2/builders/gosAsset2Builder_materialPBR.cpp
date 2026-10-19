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
	params.mat.reset();
   	

    //parse della section
	string::utf8::StringListParser sp;
    char s[1024];


    //param:diffuse_col_RGBA_HDR      e' opzionale
    if (sec->get("diffuse_col_RGBA_HDR", s, sizeof(s)))
    {
		sp.toStart(s, ",");
		if (!sp.extract_f32Array (params.mat.diffuse_col_RGBA_HDR, 4))
        {
            logger->log(eTextColor::red, "line %d => invalid value '%s' for <diffuse_col_RGBA_HDR>\n", sec->getLineStarted(), s);
            return false;
        }
	}

	//param:diffuse_texture      e' opzionale
	if (sec->get("diffuse_texture", s, sizeof(s)))
	{
		if (!asset2::asset_rtname_exists (ctx, s, &params.mat.diffuse_texture_uid))
		{
			logger->log (eTextColor::red, "line %d => invalid rtname '%s' for <diffuse_texture>\n", sec->getLineStarted(), s);
			return false;
		}		
	}

	//param:normal_texture      e' opzionale
	if (sec->get("normal_texture", s, sizeof(s)))
	{
		if (!asset2::asset_rtname_exists (ctx, s, &params.mat.normal_texture_uid))
		{
			logger->log (eTextColor::red, "line %d => invalid rtname '%s' for <normal_texture>\n", sec->getLineStarted(), s);
			return false;
		}		
	}	

    //param:metallic_factor      e' opzionale
    params.mat.metallic_factor = sec->getOrDefaultAsF32("metallic_factor", 0.0f);

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


    //All'uscita da questa fn:
    //  out_result->assetUID       			contiene l'assetUID di questo asset, gia' inserito nel DB
    //  out_result->signatureUID      		contiene la signatureUID a cui questo assetUID punta
    //  out_result->result                  vale <eBuildResult::just_built> se e' necessario creare fisicamente l'asset, altrimenti vale <eBuildResult::was_already_built>
    if (!prot_setupAsset (ctx, &params, sizeof(Params), uid_of_iniFile, sec, out_result))
        return false;

    //aggiungo le dipendenze di questo assetUID dagli assetUID delle texture
	if (!prot_add_asset_to_asset_dependency (ctx, out_result, params.mat.diffuse_texture_uid, NULL))	return false;
	if (!prot_add_asset_to_asset_dependency (ctx, out_result, params.mat.normal_texture_uid, NULL))	return false;


    //a questo punto devo davvero creare il file dell'asset
    if (doCreateAnAssetFile && eBuildResult::just_built == out_result->result)
    {
        char filenameDST[1024];
        signature_manufacture_fullFilename (ctx, out_result->signatureUID, filenameDST, sizeof(filenameDST));
		return params.mat.save (filenameDST);
    }

	return true;
}



