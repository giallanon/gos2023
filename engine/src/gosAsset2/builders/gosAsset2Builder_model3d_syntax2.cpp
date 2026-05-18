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
Builder_model3d::Syntax2::Syntax2 (): BuilderInterface (eAssetType::model3d)
{
	localAllocator = gos::getSysHeapAllocator();

	listof_UID_of_virtual_shape_that_I_need.setup (localAllocator, 16);
	listof_UID_of_concrete_shape_that_I_need.setup (localAllocator, 16);

	parsed_params.listof_shapeInfo.setup (localAllocator, 256);
	parsed_params.listof_meshes.setup (localAllocator, 256);
	parsed_params.skeleton_name = NULL;
}

//************************************
Builder_model3d::Syntax2::~Syntax2()
{
	priv_reset_parsed_params();
	parsed_params.listof_shapeInfo.unsetup ();
}

//************************************
void Builder_model3d::Syntax2::priv_reset_parsed_params()
{
	listof_UID_of_virtual_shape_that_I_need.reset();
	listof_UID_of_concrete_shape_that_I_need.reset();
	uid_of_concrete_skeleton.setInvalid();

	//shape info
	for (u32 i=0; i<parsed_params.listof_shapeInfo.getNElem(); i++)
	{
		GOSFREE(localAllocator, parsed_params.listof_shapeInfo[i].my_shape_name);
		GOSFREE(localAllocator, parsed_params.listof_shapeInfo[i].src_shape_name);
	}
	parsed_params.listof_shapeInfo.reset();

	//mesh info
	for (u32 i=0; i<parsed_params.listof_meshes.getNElem(); i++)
	{
		GOSFREE(localAllocator, parsed_params.listof_meshes[i].bone_name);
	}
	parsed_params.listof_meshes.reset();

	//skeleton info
	if (NULL != parsed_params.skeleton_name)
	{
		GOSFREE(localAllocator, parsed_params.skeleton_name);
		parsed_params.skeleton_name = NULL;
	}
	
}

//************************************
bool Builder_model3d::Syntax2::priv_extractParams (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename)
{
    //setto i default
    priv_reset_parsed_params();

    //parse della section
    char s[1024];
	char s2[1024];
	string::utf8::StringListParser sp;

	//shape info
    u32 n = 0;
	while (1)
	{
		sprintf_s (s2, sizeof(s2), "shape@%d@", n);
    	if (!sec->get(s2, s, sizeof(s)))
			break;

		sShapeInfo shapeInfo;
		shapeInfo.my_shape_name = NULL;

		//La sintassi accettata e'
		//	<rtname-of-imported-3dmodel>.<name>
		//	oppure
		//	<rtname-of-imported-3dmodel>.<name> as <my-shape-name>
		//
		// Nel primo caso, creo uno shape di nome <name> basata sulla shape importata di nome <name>
		// Nel 2nd0 caso, creo uno shape di nome <my-shape-name> basata sulla shape importata di nome <name>
		string::utf8::Iter iter;
		string::utf8::Iter iter2;
		iter.setup (s);

		if (!string::utf8::extractValue (iter, &iter2))
		{
			logger->log (eTextColor::red, "line %d, parsing [shape] =>  expected at least one values, found none\n", sec->getLineStarted());
			return false;
		}
		iter2.copyAllStr(s2, sizeof(s2));
		shapeInfo.src_shape_name = string::utf8::allocStr (localAllocator, s2);

		//questo primo parametro deve essere nella forma <rtname-of-imported-3dmodel>.<name>
		char firstparam_part1[256];
		char firstparam_part2[256];
		sp.toStart (shapeInfo.src_shape_name, '.');
		sp.next(firstparam_part1, sizeof(firstparam_part1));
		if (!sp.next(firstparam_part2, sizeof(firstparam_part2)))
		{
			logger->log (eTextColor::red, "line %d, parsing [shape] =>  first params must be in the <rtname-of-imported-3dmodel>.<name>\n", sec->getLineStarted());
			return false;
		}
		
		//a seguire, potrebbe esserci " as <my-shape-name>"
		iter.toNextValidChar();

		if (firstparam_part2[0] == '*')
		{
			if (!iter.getCurChar().isEOF())
			{
				logger->log (eTextColor::red, "line %d, parsing [shape] => invalid token after %s\n", sec->getLineStarted(), shapeInfo.src_shape_name);
				return false;
			}
		}
		else
		{
			if (!iter.getCurChar().isEOF())
			{
				if (string::utf8::extractValue (iter, &iter2))
				{
					if (!iter2.cmp("as", false))
					{
						logger->log (eTextColor::red, "line %d, parsing [shape] =>  expected ' as ' after '%s'\n", sec->getLineStarted(), shapeInfo.src_shape_name);
						return false;
					}					
					iter.toNextValidChar();
					
					if (!string::utf8::extractValue (iter, &iter2))
					{
						logger->log (eTextColor::red, "line %d, parsing [shape] =>  expected <my-shape-name>>  after ' as '\n", sec->getLineStarted());
						return false;
					}
					if (0 == iter2.totalLenghtInBytes())
					{
						logger->log (eTextColor::red, "line %d, parsing [shape] =>  expected <my-shape-name>>  after ' as '\n", sec->getLineStarted());
						return false;
					}

					iter2.copyAllStr(s2, sizeof(s2));
					shapeInfo.my_shape_name = string::utf8::allocStr (localAllocator, s2);
				}
			}

			//se non e' stato specificato un nome per <my-shape-name>, gli assegno il nome <name> derivato dal primo parametro
			if (NULL == shapeInfo.my_shape_name)
			{
				shapeInfo.my_shape_name = string::utf8::allocStr (localAllocator, firstparam_part2);
			}
		}

		//se il primo parametro ha un asterisco come nome della shape, allora vuol dire
		//che devo includere tutte le shape del modello importato
		if (firstparam_part2[0] == '*')
		{
			assert (NULL == shapeInfo.my_shape_name);
			sprintf_s (s, sizeof(s), "SELECT UID,rtname FROM " GOS_ASSET2__TABLE_VIRTUAL_ASSET " WHERE rtname LIKE '%s.%%'", firstparam_part1);
			GOSFREE(localAllocator, shapeInfo.src_shape_name);

			db::RST rst;
			if (asset2::dbcontext_query (ctx, s, rst))
			{
				while (rst.fetchRow())
				{
					UID uid;
					uid._uid = rst.getValAsU64(0);

					if (uid.isAVirtualAssetOfType(eAssetType::shape))
					{
						sShapeInfo info;
						info.src_shape_name = string::utf8::allocStr (localAllocator, rst.getVal(1));

						sp.toStart(info.src_shape_name, '.');
						sp.next(s2, sizeof(s2));
						sp.next(s2, sizeof(s2));
						info.my_shape_name = string::utf8::allocStr (localAllocator, s2);

						parsed_params.listof_shapeInfo.append(info);
					}
				}
			}
		}
		else
		{
			parsed_params.listof_shapeInfo.append(shapeInfo);
		}

		n++;
    }
	if (0 == n)
	{
		logger->log (eTextColor::red, "at least on [shape] must be defined\n");
		return false;
	}


	//skeleton
	{
		if (!sec->get("skeleton", s, sizeof(s)))
		{
			logger->log(eTextColor::red, "line %d => can't find param <skeleton>\n", sec->getLineStarted());
			return false;
		}
		strcat_s (s, sizeof(s), ".skeleton0");
		parsed_params.skeleton_name = string::utf8::allocStr (localAllocator, s);
	}



	//mesh info
    n = 0;
	while (1)
	{
		sprintf_s (s2, sizeof(s2), "mesh@%d@", n);
    	if (!sec->get(s2, s, sizeof(s)))
			break;

		sMeshInfo meshInfo;

		sp.toStart (s, ';');
		
		//nome della "my-shape"
		{
			if (!sp.next(s2, sizeof(s2)))
			{
				logger->log (eTextColor::red, "line %d, parsing [mesh] =>  expected 4 values separated by ;\n", sec->getLineStarted());
				return false;
			}

			parsed_params.listof_shapeInfo.forEach ( [&meshInfo, &s2] (u32 index, const sShapeInfo &shapeInfo)
			{
				if (string::utf8::areEqual(s2, shapeInfo.my_shape_name, true))
				{
					meshInfo.my_shape_index = index;
					return false;
				}
				return true;
			});

			if (u32MAX == meshInfo.my_shape_index)
			{
				logger->log (eTextColor::red, "line %d, parsing [mesh] => mesh of name '%s' is not defined\n", sec->getLineStarted(), s2);
				return false;
			}
		}

		
		//nome del material
		{
			if (!sp.next(s2, sizeof(s2)))
			{
				logger->log (eTextColor::red, "line %d, parsing [mesh] =>  expected 4 values separated by ;\n", sec->getLineStarted());
				return false;
			}

			if (!string::utf8::areEqual(s2, "default", true))
			{
				logger->log (eTextColor::red, "line %d, parsing [mesh] => material of name '%s' is not defined\n", sec->getLineStarted(), s2);
				return false;
			}
			meshInfo.my_material_index = u32MAX;
		}

		//nome della bone
		{
			if (!sp.next(s2, sizeof(s2)))
			{
				logger->log (eTextColor::red, "line %d, parsing [mesh] =>  expected 4 values separated by ;\n", sec->getLineStarted());
				return false;
			}
			meshInfo.bone_name = string::utf8::allocStr (localAllocator, s2);
		}
		
		//local matrix
		{
			if (!sp.next(s2, sizeof(s2)))
			{
				logger->log (eTextColor::red, "line %d, parsing [mesh] =>  expected 4 values separated by ;\n", sec->getLineStarted());
				return false;
			}		
			if (!string::utf8::areEqual(s2, "identity", true))
			{
				logger->log (eTextColor::red, "line %d, parsing [mesh] => local-matrix can only be 'identity'\n", sec->getLineStarted());
				return false;
			}
		}

		parsed_params.listof_meshes.append(meshInfo);

		n++;
    }
	if (0 == n)
	{
		logger->log (eTextColor::red, "at least on [mesh] must be defined\n");
		return false;
	}

    return true;
}

//************************************
bool Builder_model3d::Syntax2::build_begin (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFileIN, const gos::IniFileSection *secIN)
{
    assert (ctx.isValid());
	assert (NULL != secIN);
	uid_of_iniFile = uid_of_iniFileIN;
	sec = secIN;

	//parse della sezione
	if (!priv_extractParams (ctx, listof_UID_of_known_ini_file, absFilename))
    {
        logger->log (eTextColor::red, "error parsing IniFileSection\n");
        return false;
    }

	//i parametri parsati indicando uno o piu' "virtual-shape" da cui io dipendo.
	//Verifico che questi esistano nel DB
	for (u32 i=0; i<parsed_params.listof_shapeInfo.getNElem(); i++)
	{
		const sShapeInfo *shape_info = &parsed_params.listof_shapeInfo(i);

		UID uid_virtual_asset;
		UID uid_of_iniFile;
		UID uid_concrete_asset;
		if (!virtasset_rtname_exists (ctx, shape_info->src_shape_name, &uid_virtual_asset, &uid_of_iniFile, &uid_concrete_asset))
		{
			logger->log (eTextColor::red, "shape %d need '%s' which is not a valid virtual asset\n", i, shape_info->src_shape_name);
			return false;
		}

		//il rtname del virtual-asset utilizzato dalla [shape] esiste nel DB.
		//Vediamo se e' stato definito in un iniFile che io conosco
		if (!listof_UID_of_known_ini_file.exists(uid_of_iniFile))
		{
			logger->log (eTextColor::red, "shape %d need '%s' which is not a know asset for this iniFile\n", i, shape_info->src_shape_name);
			return false;
		}

		//tutto bene, mi segno che io dipendo da questo virtual-asset
		parsed_params.listof_shapeInfo[i].uid_of_concrete_shape_asset = uid_concrete_asset;
		if (listof_UID_of_virtual_shape_that_I_need.insertIfNotExists(uid_virtual_asset))
			listof_UID_of_concrete_shape_that_I_need.append(uid_concrete_asset);
	}



	//lo stesso dicasi per lo skeleton
	{
		UID uid_of_iniFile;
		if (!virtasset_rtname_exists (ctx, parsed_params.skeleton_name, &uid_of_virtual_skeleton, &uid_of_iniFile, &uid_of_concrete_skeleton))
		{
			logger->log (eTextColor::red, "skeleton '%s' is not a valid virtual asset\n", parsed_params.skeleton_name);
			return false;
		}

		//il rtname del virtual-asset utilizzato esiste nel DB.
		//Vediamo se e' stato definito in un iniFile che io conosco
		if (!listof_UID_of_known_ini_file.exists(uid_of_iniFile))
		{
			logger->log (eTextColor::red, "skeleton '%s' is not a know asset for this iniFile\n", parsed_params.skeleton_name);
			return false;
		}
	}

	return true;
}

//************************************
bool Builder_model3d::Syntax2::build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result)
{
	assert (NULL != out_bCallMeAgain);
    assert (NULL != out_result);
	*out_bCallMeAgain = false;
    out_result->reset();


	//durante "build_begin" ho determinato che le shape e lo skeleton da cui io dipendo sono validi asset.
	//Carico lo skeleton
	assert (uid_of_concrete_skeleton.isAnAssetOfType(eAssetType::skeleton));
	Skeleton skeleton;
	skeleton.reset();
	{
		char s[1024];
		asset_manufacture_fullFilename (ctx, uid_of_concrete_skeleton, s, sizeof(s));

		u32 fsize = 0;
		u8 *buffer = fs::fileLoadInMemory (localAllocator, s, &fsize);
		if (NULL == buffer)
		{
			logger->log (eTextColor::red, "unable to load skeleton '%s'\n", s);
			return false;
		}
	
		u32 n = skeleton::deserialize (buffer, fsize, localAllocator, &skeleton);
		GOSFREE(localAllocator, buffer);

		if (0 == n)
		{
			logger->log (eTextColor::red, "error loading skeleton '%s'\n", s);
			return false;
		}
	}

	//avendo in mano lo skeletro, verifico che lo bone referenziate siano valide
	skeleton::Reader skr;
	skr.setup (&skeleton);

	FastArray<sFinalMeshInfo> listof_final_meshes (localAllocator, parsed_params.listof_meshes.getNElem());
	for (u32 i=0; i<parsed_params.listof_meshes.getNElem(); i++)
	{
		sFinalMeshInfo fm;
		fm.my_material_index = parsed_params.listof_meshes(i).my_material_index;
		fm.my_shape_index = parsed_params.listof_meshes(i).my_shape_index;
		fm.bone_index = skr.bone_get_index_by_name (parsed_params.listof_meshes(i).bone_name);
		if (u32MAX == fm.bone_index)
		{
			logger->log (eTextColor::red, "mesh %d need bone '%s' which is not a valid bone for the currente selected skeleton '%s'\n", i, parsed_params.listof_meshes(i).bone_name, parsed_params.skeleton_name);
			skeleton::free (skeleton);
			return false;
		}

		listof_final_meshes.append(fm);
	}


	
	//setup di virtual-asset
	//All'uscita da questa fn:
	//  out_result->uid_virtual_asset       contiene l'UID di questo virtual asset, gia' inserito nel DB
	//  out_result->uid_concrete_asset      contiene l'UID dell'asset concreto a cui questo virtual-asset punta
	//  out_result->result                  vale <eBuildResult::just_built> se e' necessario creare fisicamente il concrete-asset, altrimenti vale <eBuildResult::was_already_built>
	{
		//in questo caso, devo "buildare" uu buffer ad hoc che funga da 'Params'.
		//Lo buildo in base ai parsed_params.
		//Questa operazione e' necessaria perche' i parsed_params sono dinamici, la descrizione dipende da quante shape sono state definite, da quanti mesh e via dicendo
		u32 needed = 0;

		for (u32 i=0;i<parsed_params.listof_shapeInfo.getNElem(); i++)
		{
			needed += string::utf8::lengthInByte(parsed_params.listof_shapeInfo(i).my_shape_name);
			needed += string::utf8::lengthInByte(parsed_params.listof_shapeInfo(i).src_shape_name);
		}

		needed += sizeof(u64);	//uid dello skeleton
		needed += listof_final_meshes.getNElem() * ( 3 * sizeof(u32) ); //info sulle mesh


		u8 *params = GOSALLOCT(u8*, gos::getScrapAllocator(), needed);
		{
			u32 ct = 0;
			for (u32 i=0;i<parsed_params.listof_shapeInfo.getNElem(); i++)
			{
				u32 n = string::utf8::lengthInByte(parsed_params.listof_shapeInfo(i).my_shape_name);
				memcpy (&params[ct], parsed_params.listof_shapeInfo(i).my_shape_name, n);
				ct += n;

				n = string::utf8::lengthInByte(parsed_params.listof_shapeInfo(i).src_shape_name);
				memcpy (&params[ct], parsed_params.listof_shapeInfo(i).src_shape_name, n);
				ct += n;
			}
			
			ct += utils::bufferWriteU64 (&params[ct], uid_of_concrete_skeleton._uid);
			for (u32 i=0; i<listof_final_meshes.getNElem(); i++)
			{
				ct += utils::bufferWriteU32 (&params[ct], listof_final_meshes(i).my_shape_index);
				ct += utils::bufferWriteU32 (&params[ct], listof_final_meshes(i).my_material_index);
				ct += utils::bufferWriteU32 (&params[ct], listof_final_meshes(i).bone_index);
			}

			assert (ct == needed);
		}

		bool ret = prot_setupVirtualAsset (ctx, &params, needed, uid_of_iniFile, sec, out_result);
		GOSFREE(gos::getScrapAllocator(), params);
		
		skeleton::free (skeleton);
		if (!ret)
			return false;
	}
	

	//i parametri parsati indicando uno o piu' "imported-model" da cui io dipendo.
	//Aggiungo le dipendenze
	{
		//skeletro
		if (!dependency_add (ctx, out_result->uid_virtual_asset, uid_of_virtual_skeleton))		return false;
		if (!dependencyRT_add (ctx, out_result->uid_concrete_asset, uid_of_concrete_skeleton))	return false;

		//shape
		bool err = false;
		listof_UID_of_virtual_shape_that_I_need.forEach ( [&ctx, out_result, &err](u32 index, const UID uid)
		{
			if (!dependency_add (ctx, out_result->uid_virtual_asset, uid))
			{
				err = true;
				return false;
			}
			return true;
		});
		if (err)
			return false;

		listof_UID_of_concrete_shape_that_I_need.forEach ( [&ctx, out_result, &err](u32 index, const UID uid)
		{
			if (!dependencyRT_add (ctx, out_result->uid_concrete_asset, uid))
			{
				err = true;
				return false;
			}			
			return true;
		});
		if (err)
			return false;
	}
	
	//E' tempo di creare l'asset
	if (doCreateAnAssetFile && eBuildResult::just_built == out_result->result)
	{
		char filenameDST[1024];
		asset_manufacture_fullFilename (ctx, out_result->uid_concrete_asset, filenameDST, sizeof(filenameDST));
		
		return priv_do_create_assetFile (ctx, out_result->uid_concrete_asset, filenameDST, listof_final_meshes);
	}

	
	return true;
}

//************************************
bool Builder_model3d::Syntax2::priv_do_create_assetFile (DBContext &ctx, UID uid_concrete_asset, const char *filenameDST, const FastArray<sFinalMeshInfo> &listof_final_meshes) const
{
	AssetFile_model3D	assetFile;

	assetFile.begin (localAllocator);

	//skeleton
	assetFile.skeleton_set (uid_of_concrete_skeleton);

	//shapes
	for (u32 i=0; i<listof_UID_of_concrete_shape_that_I_need.getNElem(); i++)
		assetFile.shape_add (listof_UID_of_concrete_shape_that_I_need(i));

	//material
	//TODO

	//meshes
	for (u32 i=0; i<listof_final_meshes.getNElem(); i++)
	{
		//faccio il match tra "my-shape-index" e la lista delle shape concrete che ho appena salvato
		u32 k = listof_final_meshes(i).my_shape_index;
		UID uid = parsed_params.listof_shapeInfo(k).uid_of_concrete_shape_asset;
		u32 index_of_concrete_shape = listof_UID_of_concrete_shape_that_I_need.simpleSearch (uid);

		assetFile.mesh_add (index_of_concrete_shape, listof_final_meshes(i).bone_index, listof_final_meshes(i).my_material_index);
		
	}

	return assetFile.save (filenameDST);
}




