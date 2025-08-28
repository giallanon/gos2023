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
        childUID._uid = rst.getColValueAsU64(0);

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
                eResType resType = static_cast<eResType> (rst2.getColValueAsU8(0));
                const char *resName = rst2.getColValue(1);

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
            sprintf_s (s, sizeof(s), "SELECT type, name FROM assetList LEFT JOIN rtnameList ON assetList.UID = rtnameList.assetUID WHERE assetList.UID=%" PRIu64 " ORDER BY type, name", childUID._uid);
            db::query (ctx.db, s, &rst2);

            u32 n = 0;
            while (rst2.fetchRow())
            {
                if (n == 0)
                {
                    ++n;
                    eAssetType childAssType = static_cast<eAssetType> (rst2.getColValueAsU8(0));
                    out << STRFMT("%-12s", asset::enumToString (childAssType)) << " | ";
                }

                const char *childAssName = rst2.getColValue(1);
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
        childUID._uid = rst.getColValueAsU64(0);
        
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
                eResType resType = static_cast<eResType> (rst2.getColValueAsU8(0));
                const char *resName = rst2.getColValue(1);

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
            sprintf_s (s, sizeof(s), "SELECT type, name FROM " GOS_ASSET__TABLE_ASSET_LIST " LEFT JOIN " GOS_ASSET__TABLE_RUNTIME_NAME " ON assetList.UID = rtnameList.assetUID WHERE assetList.UID=%" PRIu64 " ORDER BY type, name", childUID._uid);
            db::query (ctx.db, s, &rst2);

            u32 n = 0;
            while (rst2.fetchRow())
            {
                if (n == 0)
                {
                    ++n;
                    eAssetType childAssType = static_cast<eAssetType> (rst2.getColValueAsU8(0));
                    out << STRFMT("%-12s", asset::enumToString (childAssType)) << " | ";
                }

                const char *childAssName = rst2.getColValue(1);
                out << "\"" << childAssName << "\" ";
            }
        }
        out << "\n";

        //ricorsione sui figli dei figli
        Builder__print_requiredBy (out, ctx, childUID, indentIN+1);        
    }

    return num_required;
}

//***********************************
void Builder::print_dependencies (const char *baseFolder)
{
    Context ctx;
    if (!asset::context_open (baseFolder, &ctx))
    {
        logger::err ("Can't open context in %s\n", baseFolder);
        return;
    }

    asset::HashedUIDList listUID;
    listUID.setup (gos::getScrapAllocator(), 64);    

    gos::UTF8String out;
    gos::UTF8String out2;
    out.prealloc (4096);
    out2.prealloc (1024);


    out << "\n\n"
        << "========================== ASSETS LIST ==========================\n\n"
        << "Asset UID        | Asset type   | runtimeNameList\n";

    db::RST rst;
    db::RST rstAssetList;
    char s[1024];
    db::query (ctx.db, "\
 SELECT UID,type FROM " GOS_ASSET__TABLE_ASSET_LIST "\
 UNION\
 SELECT UID,type FROM " GOS_ASSET__TABLE_RES_LIST "\
 ORDER BY UID", &rstAssetList);
    while (rstAssetList.fetchRow())
    {
        asset::UID uid;
        uid._uid = rstAssetList.getColValueAsU64(0);
        
        //asset ID
        out << "-----------------+--------------+--------------------------------\n"
            << STRFMT("%016" PRIX64 "", uid._uid) << " | ";

        //asset type
        if (uid.isAnAsset())
        {
            eAssetType assType = static_cast<eAssetType> (rstAssetList.getColValueAsU8(1));
            out << STRFMT("%-12s", asset::enumToString (assType)) << " | ";
        }
        else
        {
            eResType resType = static_cast<eResType> (rstAssetList.getColValueAsU8(1));
            out << STRFMT("%-12s", asset::enumToString (resType)) << " | ";
        }

        //lista dei nomi runtime di questo asset
        if (uid.isAnAsset())
        {
            sprintf_s (s, sizeof(s), "SELECT name FROM " GOS_ASSET__TABLE_RUNTIME_NAME " WHERE assetUID=%" PRIu64 " ORDER BY name", uid._uid);
            db::query (ctx.db, s, &rst);
            while (rst.fetchRow())
            {
                out << "\"" << rst.getColValue(0) << "\" ";
            }
        }
        else
        {
            //nome della risorsa
            sprintf_s (s, sizeof(s), "SELECT name FROM " GOS_ASSET__TABLE_RES_LIST " WHERE UID=%" PRIu64 " ORDER BY name", uid._uid);
            db::query (ctx.db, s, &rst);
            if (rst.fetchRow())
                out << "\"" << rst.getColValue(0) << "\" ";
        }
        out << "\n";
        

        //lista delle dipendenze
        if (uid.isAnAsset())
        {
            out2.clear();
            Builder__print_dependencies (out2, ctx, uid, 1);
            asset::asset_get_dependecies_list (ctx, uid, true, &listUID);
            {
                auto theList = listUID._queryList();

                out << "  depends on (" << theList->getNElem() <<"): ";
                for (u32 i=0; i<theList->getNElem(); i++)
                {
                    out << STRFMT("%016" PRIX64 "", theList->queryElem(i).key._uid) << "  ";
                }
                out << "\n";
                out << out2;
            }
        }


        //lista degli asset che lo richiedono
        out2.clear();
        Builder__print_requiredBy (out2, ctx, uid, 1);
        {
            if (uid.isAnAsset())
                asset::asset_get_requireBy_list (ctx, uid, true, &listUID);
            else
                asset::res_get_requireBy_list (ctx, uid, true, &listUID);

                
            auto theList = listUID._queryList();

            out << "  required by (" << theList->getNElem() <<"): ";
            for (u32 i=0; i<theList->getNElem(); i++)
            {
                out << STRFMT("%016" PRIX64 "", theList->queryElem(i).key._uid) << "  ";
            }
            out << "\n";
            out << out2;
        }        


        out << "\n";
    }

    printf ("%s\n\n", out.getBuffer());

    asset::context_close(ctx);
}
