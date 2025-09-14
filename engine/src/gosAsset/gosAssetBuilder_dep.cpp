#include "gosAssetBuilder.h"
#include "gos.h"
#include "string/gosUTF8String.h"


using namespace gos;
using namespace gos::asset;


//***********************************
static u32 Builder__print_dependencies (gos::UTF8String &out, Context &ctx, const asset::UID &uid, u32 indentIN)
{
    char indent[32];
    memset (indent, 0, sizeof(indent));
    if (indentIN > 0)
    {
        if (indentIN >= sizeof(indent))
            indentIN = sizeof(indent) - 1;
        memset (indent, '\t', indentIN);
    }

    char s[1024];
    db::RST rst;
    u32 num_dep = 0;

    sprintf_s (s, sizeof(s), "SELECT childUID FROM " GOS_ASSET__TABLE_DEPENDS " WHERE UID=%" PRIu64 " ORDER BY childUID", uid._uid);
    db::query (ctx.db, s, &rst);
    while (rst.fetchRow())
    {
        ++num_dep;

        asset::UID childUID;
        childUID._uid = rst.getValAsU64(0);

        //uid, tipo di asset
        char childType[16];
        if (childUID.isAnAsset())
            sprintf_s (childType, sizeof(childType), "asset");
        else
            sprintf_s (childType, sizeof(childType), "resource");


        out << indent << STRFMT("%016" PRIX64 "", childUID._uid) << " | "
            << STRFMT("%-8s", childType) << " | ";

        //dettagli (elenco dei nomi runtime)
        db::RST rst2;
        if (childUID.isAResource())
        {
            sprintf_s (s, sizeof(s), "SELECT type,name FROM " GOS_ASSET__TABLE_RES_LIST " WHERE UID=%" PRIu64 "", childUID._uid);
            db::query (ctx.db, s, &rst2);
            if (rst2.fetchRow())
            {
                eResType resType = static_cast<eResType> (rst2.getValAsU8(0));
                const char *resName = rst2.getVal(1);

                out << STRFMT("%-12s", asset::enumToString (resType)) << " | "
                    << "\"" << resName << "\"";
            }
            else
            {
                out << "!!ERROR!!";
            }
        }
        else
        {
            //asset dichiarto in:
            sprintf_s (s, sizeof(s), "SELECT type, src FROM " GOS_ASSET__TABLE_ASSET_LIST " WHERE UID=%" PRIu64 "", childUID._uid);
            db::query (ctx.db, s, &rst2);
            rst2.fetchRow();
            {
                eAssetType childAssType = static_cast<eAssetType> (rst2.getValAsU8(0));
                out << STRFMT("%-12s", asset::enumToString (childAssType)) << " | "
                    << STRFMT("%-26s",rst2.getVal(1)) << " | ";
            }

            //elenco dei runtimeName
            sprintf_s (s, sizeof(s), "SELECT name FROM assetList LEFT JOIN " GOS_ASSET__TABLE_RUNTIME_NAME " ON assetList.UID = rtnameList.assetUID WHERE assetList.UID=%" PRIu64 " ORDER BY type, name", childUID._uid);
            db::query (ctx.db, s, &rst2);

            while (rst2.fetchRow())
            {
                const char *childAssName = rst2.getVal(0);
                out << "\"" << childAssName << "\" ";
            }
        }
        out << "\n";

        //ricorsione sui figli dei figli
        Builder__print_dependencies (out, ctx, childUID, indentIN+1);
    }
    
    return num_dep;
}

//***********************************
static u32 Builder__print_requiredBy  (gos::UTF8String &out, Context &ctx, const asset::UID &padreUID, u32 indentIN)
{
    char indent[32];
    memset (indent, 0, sizeof(indent));
    if (indentIN > 0)
    {
        if (indentIN >= sizeof(indent))
            indentIN = sizeof(indent) - 1;
        memset (indent, '\t', indentIN);
    }

    char s[1024];
    db::RST rst;
    u32 num_required = 0;

    sprintf_s (s, sizeof(s), "SELECT UID FROM " GOS_ASSET__TABLE_DEPENDS " WHERE childUID=%" PRIu64 " ORDER BY childUID", padreUID._uid);
    db::query (ctx.db, s, &rst);
    while (rst.fetchRow())
    {
        ++num_required;

        asset::UID childUID;
        childUID._uid = rst.getValAsU64(0);
        
        //uid, tipo di asset
        char childType[16];
        if (childUID.isAnAsset())
            sprintf_s (childType, sizeof(childType), "asset");
        else
            sprintf_s (childType, sizeof(childType), "resource");


        out << indent << STRFMT("%016" PRIX64 "", childUID._uid) << " | "
            << STRFMT("%-8s", childType) << " | ";

        //dettagli (elenco dei nomi runtime)
        db::RST rst2;
        if (childUID.isAResource())
        {
            sprintf_s (s, sizeof(s), "SELECT type,name FROM " GOS_ASSET__TABLE_RES_LIST " WHERE UID=%" PRIu64 "", childUID._uid);
            db::query (ctx.db, s, &rst2);
            if (rst2.fetchRow())
            {
                eResType resType = static_cast<eResType> (rst2.getValAsU8(0));
                const char *resName = rst2.getVal(1);

                out << STRFMT("%-12s", asset::enumToString (resType)) << " | "
                    << "\"" << resName << "\"";
            }
            else
            {
                out << "!!ERROR!!";
            }
        }
        else
        {
            //asset dichiarto in:
            sprintf_s (s, sizeof(s), "SELECT type, src FROM " GOS_ASSET__TABLE_ASSET_LIST " WHERE UID=%" PRIu64 "", childUID._uid);
            db::query (ctx.db, s, &rst2);
            rst2.fetchRow();
            {
                eAssetType childAssType = static_cast<eAssetType> (rst2.getValAsU8(0));
                out << STRFMT("%-12s", asset::enumToString (childAssType)) << " | "
                    << STRFMT("%-26s",rst2.getVal(1)) << " | ";
            }

            //elenco dei runtimeName
            sprintf_s (s, sizeof(s), "SELECT name FROM assetList LEFT JOIN " GOS_ASSET__TABLE_RUNTIME_NAME " ON assetList.UID = rtnameList.assetUID WHERE assetList.UID=%" PRIu64 " ORDER BY type, name", childUID._uid);
            db::query (ctx.db, s, &rst2);

            while (rst2.fetchRow())
            {
                const char *childAssName = rst2.getVal(0);
                out << "\"" << childAssName << "\" ";
            }
        }
        out << "\n";

        //ricorsione sui figli dei figli
        Builder__print_requiredBy (out, ctx, childUID, indentIN+1);        
    }

    return num_required;
}

static void Builder__do_print (asset::Context &ctx, gos::UTF8String &out, db::RST &rstAssetList)
{
    asset::FastUIDList fastUIDList(gos::getScrapAllocator(), 64);    
    asset::HashedUIDList listUID;
    listUID.setup (gos::getScrapAllocator(), 64);    

    gos::UTF8String out2;
    out2.prealloc (1024);

    while (rstAssetList.fetchRow())
    {
        asset::UID uid;
        uid._uid = rstAssetList.getValAsU64(0);
        
        //asset/resource ID
        out << "-----------------+--------------+--------------------------------\n"
            << STRFMT("%016" PRIX64 "", uid._uid) << " | ";

        //asset/resource type
        if (uid.isAnAsset())
        {
            eAssetType assType = static_cast<eAssetType> (rstAssetList.getValAsU8(1));
            out << STRFMT("%-12s", asset::enumToString (assType)) << " | ";
        }
        else
        {
            eResType resType = static_cast<eResType> (rstAssetList.getValAsU8(1));
            out << STRFMT("%-12s", asset::enumToString (resType)) << " | ";
        }

        db::RST rst;
        char s[256];

        if (uid.isAnAsset())
        {
            //file src dell'asset
            sprintf_s (s, sizeof(s), "SELECT src FROM " GOS_ASSET__TABLE_ASSET_LIST " WHERE UID=%" PRIu64 "", uid._uid);
            db::query (ctx.db, s, &rst);
            rst.fetchRow();
            out << STRFMT("%-26s",rst.getVal(0)) << " | ";

            //lista dei nomi runtime di questo asset
            sprintf_s (s, sizeof(s), "SELECT name FROM " GOS_ASSET__TABLE_RUNTIME_NAME " WHERE assetUID=%" PRIu64 " ORDER BY name", uid._uid);
            db::query (ctx.db, s, &rst);
            while (rst.fetchRow())
            {
                out << "\"" << rst.getVal(0) << "\" ";
            }
        }
        else
        {
            //nome-del-file-src/nome-della risorsa
            sprintf_s (s, sizeof(s), "SELECT name FROM " GOS_ASSET__TABLE_RES_LIST " WHERE UID=%" PRIu64 " ORDER BY name", uid._uid);
            db::query (ctx.db, s, &rst);
            if (rst.fetchRow())
                out << "\"" << rst.getVal(0) << "\" ";
        }
        out << "\n";
        

        //lista delle dipendenze
        out2.clear();
        Builder__print_dependencies (out2, ctx, uid, 1);
        asset::asset_get_dependecies_list (ctx, uid, true, &listUID);
        {
            auto theList = listUID._queryList();

            out << "\n" << "  depends on (" << theList->getNElem() <<"): ";
            for (u32 i=0; i<theList->getNElem(); i++)
            {
                out << STRFMT("%016" PRIX64 "", theList->queryElem(i).key._uid) << "  ";
            }
            out << "\n";
            out << out2;
        }


        //lista degli asset che lo richiedono
        out2.clear();
        Builder__print_requiredBy (out2, ctx, uid, 1);
        {
            if (uid.isAnAsset())
                asset::asset_get_requireBy_list (ctx, uid, true, &listUID, eFilter::both);
            else
                asset::res_get_requireBy_list (ctx, uid, true, &listUID, eFilter::both);

                
            auto theList = listUID._queryList();

            out << "\n" << "  required by (" << theList->getNElem() <<"): ";
            for (u32 i=0; i<theList->getNElem(); i++)
            {
                out << STRFMT("%016" PRIX64 "", theList->queryElem(i).key._uid) << "  ";
            }
            out << "\n";
            out << out2;
        }        

        //lista delle "dipendenze runtime"
        if (uid.isAnAsset())
        {
            asset::asset_get_runtime_dependecies_list (ctx, uid, true, &fastUIDList);

            out << "\n" << "  runtime dep list: ";
            for (u32 i=0; i<fastUIDList.getNElem(); i++)
            {
                out << STRFMT("%016" PRIX64 "", fastUIDList(i)._uid) << "  ";
            }
            out << "\n";            
        }

        out << "\n";
    }


}

//***********************************
void Builder::get_dependencies_report (gos::UTF8String &out, const char *baseFolder, asset::eFilter filter)
{
    Context ctx;
    if (!asset::context_open (baseFolder, &ctx))
    {
        logger::err ("Can't open context in %s\n", baseFolder);
        return;
    }

    db::RST rstAssetList;
    if (eFilter::only_resources == filter || eFilter::both == filter)
    {
        out << "\n\n"
            << "========================== RESOURCES LIST ==========================\n\n"
            << "Resource UID     | Type         | Name\n";

        db::query (ctx.db, "SELECT UID,type FROM " GOS_ASSET__TABLE_RES_LIST " ORDER BY UID", &rstAssetList);
        Builder__do_print (ctx, out, rstAssetList);
    }


    if (eFilter::only_assets == filter || eFilter::both == filter)
    {
        out << "\n\n"
            << "========================== ASSETS LIST ==========================\n\n"
            << "Asset UID        | Type         | Declared in                | runtimeName-list\n";

        db::query (ctx.db, "SELECT UID,type FROM " GOS_ASSET__TABLE_ASSET_LIST " ORDER BY UID", &rstAssetList);
        Builder__do_print (ctx, out, rstAssetList);
    }

    asset::context_close(ctx);
}

//***********************************
void Builder::print_dependencies_report (const char *baseFolder, asset::eFilter filter)
{
    gos::UTF8String out;
    out.prealloc (4096);

    get_dependencies_report (out, baseFolder, filter);
    printf ("%s\n\n", out.getBuffer());
}

//***********************************
void Builder::save_dependencies_report (const char *baseFolder, asset::eFilter filter)
{
    gos::UTF8String out;
    out.prealloc (4096);
    get_dependencies_report (out, baseFolder, filter);

    
    char s[512];
    memset (s, 0, sizeof(s));
    asset::asset_get_srcfolder_name (baseFolder, s, sizeof(s));
    strcat_s (s, sizeof(s), "/__dependencies.txt");

    fs::fileSaveBuffer (s, out.getBuffer(), out.lengthInByte());
}


