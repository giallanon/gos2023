#include "gosAsset2Builder_glb.h"
#include "gos.h"
#include "../gos/gosString.h"
#include "../gosAsset2Builder.h"
#include "../gos/gosBufferWriter.h"
#include "../gos/gosDataBlob.h"

using namespace gos;
using namespace gos::asset2;


//************************************
bool Builder_glb::priv_extractParams (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename)
{
    //setto i default
    memset (&params, 0, sizeof(Params));

    //parse della section
    char s[1024];

    //param:src         e' mandatorio ed indica il nome del file .glb da importare
    if (!sec->get("src", s, sizeof(s)))
    {
        logger->log(eTextColor::red, "line %d => can't find param <src>\n", sec->getLineStarted());
        return false;
    }
    if (!asset2::Builder::makeABSPathFromFilename (ctx, logger, listof_UID_of_known_ini_file, absFilename, s, params.src, sizeof(params.src)))
        return false;

    return true;
}

//************************************
bool Builder_glb::build_begin (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFileIN, const gos::IniFileSection *secIN)
{
	assert (false == buildCtx.bAModelWasImported);
    assert (ctx.isValid());
	assert (NULL != sec);
	uid_of_iniFile = uid_of_iniFileIN;
	sec = secIN;
    
    //parse della sezione
    if (!priv_extractParams(ctx, listof_UID_of_known_ini_file, absFilename))
    {
        logger->log (eTextColor::red, "error parsing IniFileSection\n");
        return false;
    }

    //il parametro src indica una risorsa eResType::model_glb da cui io dipendo
    //La risorsa deve esistere nel DB
    if (!prot_needResource (ctx, listof_UID_of_known_ini_file, eResType::model_glb, params.src, &params.uid__resource_file_glb))
    {
        logger->log (eTextColor::red, "resource [%s] '%s' not found in DB\n", asset2::enumToString(eResType::model_glb), params.src);
        return false;
    }     

    //questo file gosasset_d dipende dalla risorsa params.uid__resource_file_glb)
    if (!asset2::dependency_exists(ctx, uid_of_iniFile, params.uid__resource_file_glb))
    {
        if (!asset2::dependency_add (ctx, uid_of_iniFile, params.uid__resource_file_glb)) 
            return false;  
    }


	//recupero il mio rtname
    memset (glb_rtname, 0, sizeof(glb_rtname));
    sec->get("__value", glb_rtname, sizeof(glb_rtname));


    return true;
}

//************************************
void Builder_glb::build_end()
{
	if (buildCtx.bAModelWasImported)
	{
		buildCtx.bAModelWasImported = false;
		buildCtx.imported.free();
	}
}

//************************************
bool Builder_glb::build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result)
{
	assert (NULL != out_bCallMeAgain);
    assert (NULL != out_result);
	*out_bCallMeAgain = false;
    out_result->reset();


	if (!buildCtx.bAModelWasImported)
	{
		//e' la prima volta che si chiama build_exe()


		//setup di virtual-asset
		//All'uscita da questa fn:
		//  out_result->uid_virtual_asset       contiene l'UID di questo virtual asset, gia' inserito nel DB
		//  out_result->uid_concrete_asset      contiene l'UID dell'asset concreto a cui questo virtual-asset punta
		//  out_result->result                  vale <eBuildResult::just_built> se e' necessario creare fisicamente il concrete-asset, altrimenti vale <eBuildResult::was_already_built>
		if (!prot_setupVirtualAsset (ctx, &params, sizeof(Params), uid_of_iniFile, sec, out_result))
			return false;

		//aggiungo le dipendenze di virtual-asset dalla risorsa model_glb
		if (!dependency_add (ctx, out_result->uid_virtual_asset, params.uid__resource_file_glb)) return false;



		//importo il modello glb e mi salvo il risultato
		{
			//vertex layout desiderato
			struct Vertex 
			{
				gos::vec3f  pos;
				gos::vec3f  normal;
				gos::vec2f  tutv0;
			};			
			gos::shape::VtxLayoutWriter writer(&buildCtx.vtxLayout);
			writer.begin()
				.addPos3(offsetof(Vertex, pos))
				.addNorm3(offsetof(Vertex, normal))
				.addTexCoord(offsetof(Vertex, tutv0))
			.end();

			Importer_glb imp;
			if (!imp.importFromFile (params.src, buildCtx.vtxLayout, gos::getSysHeapAllocator(), &buildCtx.imported))
			{
				logger->log (eTextColor::red, "error importing model form %s\n", params.src);
				return false;        
			}
		}
		buildCtx.bAModelWasImported = true;
		buildCtx.whatToBuild = eWhatToBuild::shapes;
		buildCtx.iToBuild = 0;
		*out_bCallMeAgain= true;


		//a questo punto devo davvero creare il file dell'asset
		if (doCreateAnAssetFile && eBuildResult::just_built == out_result->result)
		{
			//l'asset file per questo asset e' sostanzialmente un fake, non contiene nulla.
			//Quello che mi interessa sono i "sub asset" che verranno creati alle prossime chiamate a build_exe()
			char filenameDST[1024];
			asset_manufacture_fullFilename (ctx, out_result->uid_concrete_asset, filenameDST, sizeof(filenameDST));
			
			gos::File f;
			fs::fileOpenForW (&f, filenameDST);
			fs::fileWrite (f, params.src, sizeof(params.src));
			fs::fileClose(f);

			priv_print_report (filenameDST);
		}

		return true;
	}
	else
	{
		//chiamate successive alla prima
		if (eWhatToBuild::shapes == buildCtx.whatToBuild)
		{
			if (!priv_build_shape(ctx, doCreateAnAssetFile, out_result))
				return false;

			buildCtx.iToBuild++;
			if (buildCtx.iToBuild < buildCtx.imported.numShapes)
			{
				*out_bCallMeAgain = true;
				return true;
			}

			//passiamo a buildare lo scheletro (se esiste)
			if (skeleton::isValid(buildCtx.imported.skeleton))
			{
				buildCtx.whatToBuild = eWhatToBuild::skeleton;
				buildCtx.iToBuild = 0;
				*out_bCallMeAgain = true;
			}

			return true;
		}
		else if (eWhatToBuild::skeleton == buildCtx.whatToBuild)		
		{
			if (!priv_build_skeleton(ctx, doCreateAnAssetFile, out_result))
				return false;

			//ho importato tutte le shape e anche lo skeletro, non c'e' altro da fare, ho finito
			*out_bCallMeAgain = false;
			return true;
		}
	}

	return false;
}

//************************************
bool Builder_glb::priv_build_shape (DBContext &ctx, bool doCreateAnAssetFile, sBuildResult *out_result)
{
	assert (buildCtx.bAModelWasImported);
	assert (buildCtx.whatToBuild == eWhatToBuild::shapes);
	assert (buildCtx.iToBuild < buildCtx.imported.numShapes);


	//setup di virtual-asset
	//All'uscita da questa fn:
	//  out_result->uid_virtual_asset       contiene l'UID di questo virtual asset, gia' inserito nel DB
	//  out_result->uid_concrete_asset      contiene l'UID dell'asset concreto a cui questo virtual-asset punta
	//  out_result->result                  vale <eBuildResult::just_built> se e' necessario creare fisicamente il concrete-asset, altrimenti vale <eBuildResult::was_already_built>
	params.subresource_type = eAssetType::shape;
	params.subresource_index = buildCtx.iToBuild;

	char shape_rtName[256];
	sprintf_s (shape_rtName, sizeof(shape_rtName), "%s.%s", glb_rtname, buildCtx.imported.shapeNameList[buildCtx.iToBuild]);
	if (!prot_setupVirtualAsset_ex (ctx, params.subresource_type, &params, sizeof(Params), shape_rtName, uid_of_iniFile, sec->getLineStarted(), out_result))
		return false;

	//aggiungo le dipendenze di virtual-asset dalla risorsa model_glb
	if (!dependency_add (ctx, out_result->uid_virtual_asset, params.uid__resource_file_glb)) return false;


	if (doCreateAnAssetFile && eBuildResult::just_built == out_result->result)
	{
		char filenameDST[1024];
		asset_manufacture_fullFilename (ctx, out_result->uid_concrete_asset, filenameDST, sizeof(filenameDST));
		
		const u32 i = buildCtx.iToBuild;
		const u32 n = shape::serialize (&buildCtx.imported.shapeList[i], NULL, 0);
		u8 *p = GOSALLOCT(u8*, gos::getScrapAllocator(), n);
		shape::serialize (&buildCtx.imported.shapeList[i], p, n);
		fs::fileSaveBuffer (filenameDST, p, n);
		GOSFREE(gos::getScrapAllocator(), p);
	}

	return true;
}

//************************************
bool Builder_glb::priv_build_skeleton (DBContext &ctx, bool doCreateAnAssetFile, sBuildResult *out_result)
{
	assert (buildCtx.bAModelWasImported);
	assert (buildCtx.whatToBuild == eWhatToBuild::skeleton);
	assert (buildCtx.iToBuild == 0);


	//setup di virtual-asset
	//All'uscita da questa fn:
	//  out_result->uid_virtual_asset       contiene l'UID di questo virtual asset, gia' inserito nel DB
	//  out_result->uid_concrete_asset      contiene l'UID dell'asset concreto a cui questo virtual-asset punta
	//  out_result->result                  vale <eBuildResult::just_built> se e' necessario creare fisicamente il concrete-asset, altrimenti vale <eBuildResult::was_already_built>
	params.subresource_type = eAssetType::skeleton;
	params.subresource_index = buildCtx.iToBuild;

	char skeleton_rtName[256];
	sprintf_s (skeleton_rtName, sizeof(skeleton_rtName), "%s.skeleton%d", glb_rtname, buildCtx.iToBuild);
	if (!prot_setupVirtualAsset_ex (ctx, params.subresource_type, &params, sizeof(Params), skeleton_rtName, uid_of_iniFile, sec->getLineStarted(), out_result))
		return false;

	//aggiungo le dipendenze di virtual-asset dalla risorsa model_glb
	if (!dependency_add (ctx, out_result->uid_virtual_asset, params.uid__resource_file_glb)) return false;


	if (doCreateAnAssetFile && eBuildResult::just_built == out_result->result)
	{
		char filenameDST[1024];
		asset_manufacture_fullFilename (ctx, out_result->uid_concrete_asset, filenameDST, sizeof(filenameDST));
		
		const u32 n = skeleton::serialize (buildCtx.imported.skeleton, NULL, 0);
		u8 *p = GOSALLOCT(u8*, gos::getScrapAllocator(), n);
		skeleton::serialize (buildCtx.imported.skeleton, p, n);
		fs::fileSaveBuffer (filenameDST, p, n);
		GOSFREE(gos::getScrapAllocator(), p);
	}

	return true;
}

//************************************
void Builder_glb::priv_print_report(const char *filenameDST) const
{
	gos::UTF8String out;
	out.prealloc (1024);
	
	out << "src: " << params.src << "\n";

	out << "\n\n============= VTX LAYOUT ==============\n";
	{
		out << "offset | format    | semantic   | index\n"
			<< "---------------------------------------\n";
		gos::shape::VtxLayoutReader vtxR(&buildCtx.vtxLayout);
		for (u32 i=0; i<vtxR.getNumElem(); i++)
		{
			out << STRFMT("% 6d", vtxR.getOffset(i))
				<< " | " << STRFMT("%-9s", gos::utils::enumToString(vtxR.getFormat(i)))
				<< " | " << STRFMT("%-10s", shape::enumToString(vtxR.getSemantic(i)))
				<< " | " << vtxR.getIndex(i)
				<< "\n";
		}
	}

	out << "\n\n============= SHAPE ===============\n";
	{
		out << "#   | num-vertex | num-index | name\n"
			<< "-----------------------------------\n";
		for (u32 i=0; i<buildCtx.imported.numShapes; i++)
		{
			out << STRFMT("%03d", i)
				<< " | " << STRFMT("% 10d", buildCtx.imported.shapeList[i].numVtx)
				<< " | " << STRFMT("% 9d", buildCtx.imported.shapeList[i].numIdx)
				<< " | " << buildCtx.imported.shapeNameList[i]
				<< "\n";
		}
	}

	out << "\n\n======== SKELETON ==========\n";
	{
		skeleton::debug__print (buildCtx.imported.skeleton, out);
	}


	char s[1024];
	sprintf_s (s, sizeof(s), "%s.model_info.txt", filenameDST);
	fs::fileSaveBuffer (s, out.getBuffer(), out.lengthInByte());
}	
