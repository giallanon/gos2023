#include "gos.h"
#include "../gos/gosString.h"
#include "../gosAsset2Builder.h"
#include "gosAsset2Builder_glb.h"
#include "gosAsset2Builder_glb_importer.h"

using namespace gos;
using namespace gos::asset2;


//************************************
bool Builder_glb::priv_extractParams (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, const IniFileSection *sec, Params *out_params)
{
    assert (NULL != sec);
    assert (NULL != out_params);
    
    //setto i default
    memset (out_params, 0, sizeof(Params));

    //parse della section
    char s[1024];

    //param:src         e' mandatorio ed indica il nome del file .glb da importare
    if (!sec->get("src", s, sizeof(s)))
    {
        logger->log(eTextColor::red, "line %d => can't find param <src>\n", sec->getLineStarted());
        return false;
    }
    if (!asset2::Builder::makeABSPathFromFilename (ctx, logger, listof_UID_of_known_ini_file, absFilename, s, out_params->src, sizeof(out_params->src)))
        return false;

    return true;
}

//************************************
bool Builder_glb::build (DBContext &ctx, u64 buildTime_UTC, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out_result)
{
    assert (ctx.isValid());
    assert (NULL != sec);
    assert (NULL != out_result);

    out_result->reset();

    //parse della sezione
    Params params;
    if (!priv_extractParams(ctx, listof_UID_of_known_ini_file, absFilename, sec, &params))
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


    //setup di virtual-asset
    //All'uscita da questa fn:
    //  out_result->uid_virtual_asset       contiene l'UID di questo virtual asset, gia' inserito nel DB
    //  out_result->uid_concrete_asset      contiene l'UID dell'asset concreto a cui questo virtual-asset punta
    //  out_result->result                  vale <eBuildResult::just_built> se e' necessario creare fisicamente il concrete-asset, altrimenti vale <eBuildResult::was_already_built>
    if (!prot_setupVirtualAsset (ctx, &params, sizeof(Params), uid_of_iniFile, sec, out_result))
        return false;


    //aggiungo le dipendenze di virtual-asset dalla risorsa model_glb
    if (!dependency_add (ctx, out_result->uid_virtual_asset, params.uid__resource_file_glb)) return false;


    
    //a questo punto devo davvero creare il file dell'asset
    if (doCreateAnAssetFile && eBuildResult::just_built == out_result->result)
    {
        char filenameDST[1024];
        asset_manufacture_fullFilename (ctx, out_result->uid_concrete_asset, filenameDST, sizeof(filenameDST));
        return priv_do_create_assetFile (ctx, out_result->uid_concrete_asset, params, filenameDST);
    }

    return true;
}


//************************************
bool Builder_glb::priv_do_create_assetFile (DBContext &ctx, UID uid_concrete_asset, const Params &params, const char *filenameDST)
{
    bool result = false;

    //vertex layout desiderato
    struct Vertex 
    {
        gos::vec3f  pos;
        gos::vec3f  normal;
        gos::vec2f  tutv0;
    };

    gos::VtxLayout vtxLayot;
    gos::shape::VtxLayoutWriter writer(&vtxLayot);
    writer.begin()
        .addPos3(offsetof(Vertex, pos))
        .addNorm3(offsetof(Vertex, normal))
        .addTexCoord(offsetof(Vertex, tutv0))
    .end();


    Importer_glb::Result    r;
    Importer_glb            imp;
    if (imp.importFromFile (params.src, vtxLayot, gos::getSysHeapAllocator(), &r))
    {
        r.free();
    }
    
    
    return result;
}

