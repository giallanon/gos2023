#include "gosResShaderBuilder.h"
#include "gosRes.h"
#include "gos.h"


using namespace gos;
using namespace gos::res;

//************************************
bool ShaderBuilder::build (sBuilderSession &session, const IniFileSection *sec, u64 lastTimeIniSectionWasUpdate)
{
    if (eResType::vtx_shader == getResType())
        gos::logger::log ("VtxShaderBuilder:\n");
    else
        gos::logger::log ("PxlShaderBuilder:\n");
    
    gos::logger::incIndent();
    const bool ret = priv_do_build (session, sec, lastTimeIniSectionWasUpdate);
    gos::logger::decIndent();

    return ret;
}

//************************************
bool ShaderBuilder::priv_do_build (sBuilderSession &session, const IniFileSection *sec, u64 lastTimeIniSectionWasUpdate)
{
    //parse della sezione
    sData data;
    if (!priv_parseSection(sec, &data))
        return false;

    //calcolo il resID
    //Il resID dipende solo ed esclusivamente dai dati presenti nella IniSection, ad esclusione del campo runtimeName
    //Considera i dati della iniSection come i "parametri di build della risorsa"
    //Se i parametri non sono cambiati, ogni volta otterr' sempre lo stesso UID indipendentemente dal file in cui
    //si trova la sezione, dalla sua posizione all'interno del file e da quant'altro
    //E' importante che data.params contenga solo ed esattamente quello che serve per buildare
    const u32 resUID = priv_calc_resUID (data.params);


    //a questo punto, verifico se e' davvero necessario rebuildare la risorsa





    //fullpath del file src
    char shaderSRCFile[1024];
    sprintf_s (shaderSRCFile, sizeof(shaderSRCFile), "%s/raw/shaders/%s", session.baseFolder, data.params.src);

    //se questa risorsa e' gia' stato compilata in questa sessione, evito di rifalo
    bool bRebuilt = false;
    if (res::need_rebuild (session, resUID, lastTimeIniSectionWasUpdate))
    {
        bRebuilt = true;

        //fullPath del file compilato
        char shaderDSTFile[1024];
        manufacture_compiled_fullFilePathAndName (session.baseFolder,resUID, shaderDSTFile, sizeof(shaderDSTFile));

        //buildo
        char shaderStage[8];
        if (eResType::vtx_shader == getResType())
            sprintf_s (shaderStage, sizeof(shaderStage), "vert");
        else
            sprintf_s (shaderStage, sizeof(shaderStage), "frag");    
        if (!shader_compile (shaderSRCFile, shaderStage, data.params.define, shaderDSTFile, false))
            return false;
    }

    //aggiorno il DB
    res::onResourceBuilt (session, resUID, getResType(), bRebuilt, data.runtimeName);

    eTextColor col = eTextColor::green;
    if (!bRebuilt)
        col = eTextColor::darkBlue;

    gos::logger::log (col, "UID=%08X, runtime-name=%s\n\n", resUID, data.runtimeName);
    return true;
}

//************************************
bool ShaderBuilder::priv_parseSection (const IniFileSection *sec, sData *out) const
{
    assert (NULL != sec);
    assert (NULL != out);
    
    memset (out, 0, sizeof(sData));

    //param:runtimeName
    //e' mandatorio ed indica il nome a runtime della risorsa
    if (!sec->get("runtimeName", out->runtimeName, sizeof(out->runtimeName)))
    {
        gos::logger::err ("can't find param <resName>\n");
        return false;
    }

    //param:src
    //e' mandatorio ed indica il nome dello shader in formato testo da compilare
    if (!sec->get("src", out->params.src, sizeof(out->params.src)))
    {
        gos::logger::err ("can't find param <src>\n");
        return false;
    }

    //param:define
    //e' opzionale e, se esiste, e' una sequenza di parole separate da spazio
    sec->get ("define", out->params.define, sizeof(out->params.define));

    return true;
}

//************************************
u32 ShaderBuilder::priv_calc_resUID (const Params &params) const
{
    return res::calc_resUID (getResType(), &params, sizeof(Params));
}

//************************************
bool ShaderBuilder::calc_resUID (const IniFileSection *sec, u32 *out_resUID) const
{
    assert (NULL != sec);
    assert (NULL != out_resUID);

    //parse della sezione
    sData data;
    if (priv_parseSection(sec, &data))
    {
        *out_resUID = priv_calc_resUID(data.params);
        return true;
    }
    else
    {
        *out_resUID = 0;
        return false;
    }
}