#include "gosAsset2Builder.h"
#include "gos.h"
#include "string/gosUTF8String.h"


using namespace gos;
using namespace gos::asset2;


//***********************************
static u32 Builder__print_dependencies (gos::UTF8String &out, DBContext &ctx, UID uid, u32 indentIN)
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

    sprintf_s (s, sizeof(s), "SELECT childUID FROM " GOS_ASSET2__TABLE_DEPENDS " WHERE UID=%" PRIu64 " ORDER BY childUID", uid._uid);
    db::query (ctx.db, s, &rst);
    while (rst.fetchRow())
    {
        ++num_dep;

        UID childUID;
        childUID._uid = rst.getValAsU64(0);

        //uid, tipo di asset
        char childType[16];
        if (childUID.isAnAsset())
            sprintf_s (childType, sizeof(childType), "asset");
        else if (childUID.isAResource())
            sprintf_s (childType, sizeof(childType), "resource");
        else
            sprintf_s (childType, sizeof(childType), "v-asset");


        out << indent << STRFMT("%016" PRIX64 "", childUID._uid) << " | "
            << STRFMT("%-8s", childType) << " | ";

        //dettagli (elenco dei nomi runtime)
        db::RST rst2;
        if (childUID.isAResource())
        {
            sprintf_s (s, sizeof(s), "SELECT type,abspath FROM " GOS_ASSET2__TABLE_RES " WHERE UID=%" PRIu64 "", childUID._uid);
            db::query (ctx.db, s, &rst2);
            if (rst2.fetchRow())
            {
                eResType resType = static_cast<eResType> (rst2.getValAsU8(0));
                const char *resName = rst2.getVal(1);

                out << STRFMT("%-12s", asset2::enumToString (resType)) << " | "
                    << "\"" << resName << "\"";
            }
            else
            {
                out << "!!ERROR!!";
            }
        }
        else if (childUID.isVirtualAsset())
        {
            //virtual asset dichiarto in:
            sprintf_s (s, sizeof(s), "SELECT line, UID_asset, rtname, abspath "\
"FROM " GOS_ASSET2__TABLE_VIRTUAL_ASSET " as t1 LEFT JOIN " GOS_ASSET2__TABLE_RES " as t2 ON t1.UID_ini=t2.UID "\
"WHERE t1.UID=%" PRIu64 "", childUID._uid);
            db::query (ctx.db, s, &rst2);
            rst2.fetchRow();
            {
                UID UID_concrete_asset;
                
                const eAssetType childAssType = childUID.getVirtualAssetType();
                const u32 line = rst2.getValAsU32(0);
                UID_concrete_asset._uid = rst2.getValAsU64(1);
                const char *rtname = rst2.getVal(2);
                const char *absIniPath = rst2.getVal(3);

                out << STRFMT("%-12s", asset2::enumToString (childAssType)) << " | "
                    << absIniPath << "@" << line << " | "
                    << "\"" << rtname << "\" | "
                    << STRFMT("%016" PRIX64 "", UID_concrete_asset._uid);
            }
        }
        else
        {
            //TODO
            assert (childUID.isAnAsset());
        }
        out << "\n";

        //ricorsione sui figli dei figli
        Builder__print_dependencies (out, ctx, childUID, indentIN+1);
    }
    
    return num_dep;
}

//***********************************
static u32 Builder__print_requiredBy  (gos::UTF8String &out, DBContext &ctx, UID padreUID, u32 indentIN)
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

    sprintf_s (s, sizeof(s), "SELECT UID FROM " GOS_ASSET2__TABLE_DEPENDS " WHERE childUID=%" PRIu64 " ORDER BY childUID", padreUID._uid);
    db::query (ctx.db, s, &rst);
    while (rst.fetchRow())
    {
        ++num_required;

        UID childUID;
        childUID._uid = rst.getValAsU64(0);
        
        //uid, tipo di asset
        char childType[16];
        if (childUID.isAnAsset())
            sprintf_s (childType, sizeof(childType), "asset");
        else if (childUID.isAResource())
            sprintf_s (childType, sizeof(childType), "resource");
        else
            sprintf_s (childType, sizeof(childType), "v-asset");



        out << indent << STRFMT("%016" PRIX64 "", childUID._uid) << " | "
            << STRFMT("%-8s", childType) << " | ";

        //dettagli (elenco dei nomi runtime)
        db::RST rst2;
        if (childUID.isAResource())
        {
            sprintf_s (s, sizeof(s), "SELECT type,abspath FROM " GOS_ASSET2__TABLE_RES " WHERE UID=%" PRIu64 "", childUID._uid);
            db::query (ctx.db, s, &rst2);
            if (rst2.fetchRow())
            {
                eResType resType = static_cast<eResType> (rst2.getValAsU8(0));
                const char *resName = rst2.getVal(1);

                out << STRFMT("%-12s", asset2::enumToString (resType)) << " | "
                    << "\"" << resName << "\"";
            }
            else
            {
                out << "!!ERROR!!";
            }
        }
        else if (childUID.isVirtualAsset())
        {
            //virtual asset dichiarto in:
            sprintf_s (s, sizeof(s), "SELECT line, UID_asset, rtname, abspath "\
"FROM " GOS_ASSET2__TABLE_VIRTUAL_ASSET " as t1 LEFT JOIN " GOS_ASSET2__TABLE_RES " as t2 ON t1.UID_ini=t2.UID "\
"WHERE t1.UID=%" PRIu64 "", childUID._uid);
            db::query (ctx.db, s, &rst2);
            rst2.fetchRow();
            {
                UID UID_concrete_asset;
                
                const eAssetType childAssType = childUID.getVirtualAssetType();
                const u32 line = rst2.getValAsU32(0);
                UID_concrete_asset._uid = rst2.getValAsU64(1);
                const char *rtname = rst2.getVal(2);
                const char *absIniPath = rst2.getVal(3);

                out << STRFMT("%-12s", asset2::enumToString (childAssType)) << " | "
                    << absIniPath << "@" << line << " | "
                    << "\"" << rtname << "\" | "
                    << STRFMT("%016" PRIX64 "", UID_concrete_asset._uid);
            }
        }
        else
        {
            //TODO
            assert (childUID.isAnAsset());
        }
        out << "\n";

        //ricorsione sui figli dei figli
        Builder__print_requiredBy (out, ctx, childUID, indentIN+1);        
    }

    return num_required;
}

static void Builder__do_print (DBContext &ctx, gos::UTF8String &out, db::RST &rstAssetList)
{
    asset2::FastUIDList fastUIDList(gos::getScrapAllocator(), 64);    
    asset2::UniqueUIDList listUID (gos::getScrapAllocator(), 64);    

    gos::UTF8String out2;
    out2.prealloc (1024);

    while (rstAssetList.fetchRow())
    {
        UID uid;
        uid._uid = rstAssetList.getValAsU64(0);
        
        //asset/resource ID
        out << "-----------------+--------------+--------------------------------\n"
            << STRFMT("%016" PRIX64 "", uid._uid) << " | ";

        //asset/resource type
        if (uid.isVirtualAsset())
        {
            eAssetType assType = static_cast<eAssetType> (uid.getVirtualAssetType());
            out << STRFMT("%-12s", asset2::enumToString (assType)) << " | ";
        }
        else if (uid.isAResource())
        {
            eResType resType = static_cast<eResType> (uid.getResourceType());
            out << STRFMT("%-12s", asset2::enumToString (resType)) << " | ";
        }
        else
        {
            //TODO
            assert(uid.isAnAsset());
            continue;
        }

        db::RST rst;
        char s[256];

        if (uid.isVirtualAsset())
        {
            //nome-del-file-src / runtimename
            sprintf_s (s, sizeof(s), "SELECT abspath,line,rtname FROM " GOS_ASSET2__TABLE_VIRTUAL_ASSET " as T1 LEFT JOIN " GOS_ASSET2__TABLE_RES " as T2 \
ON T1.UID_ini = T2.UID \
WHERE T1.UID=%" PRIu64 "", uid._uid);
            db::query (ctx.db, s, &rst);
            rst.fetchRow();
            out << rst.getVal(0) << "@" << rst.getValAsU32(1) << " | " << "\"" << rst.getVal(2) << "\" ";
        }
        else if (uid.isAResource())
        {
            //nome-del-file-src/nome-della risorsa
            sprintf_s (s, sizeof(s), "SELECT abspath FROM " GOS_ASSET2__TABLE_RES " WHERE UID=%" PRIu64 " ORDER BY abspath", uid._uid);
            db::query (ctx.db, s, &rst);
            if (rst.fetchRow())
                out << "\"" << rst.getVal(0) << "\" ";
        }
        else
        {
            //TODO
            assert (uid.isAnAsset());
        }
        out << "\n";
        

        //lista delle dipendenze
        out2.clear();
        Builder__print_dependencies (out2, ctx, uid, 1);
        asset2::dependency_get_dependecies_list (ctx, uid, true, &listUID);
        {
            out << "\n" << "  depends on (" << listUID.getNElem() <<"): ";
            listUID.forEach ( [&out] (u32 index, const UID uid) {
                out << STRFMT("%016" PRIX64 "", uid._uid) << "  ";
                return true;
            });
            out << "\n";
            out << out2;
        }


        //lista degli asset che lo richiedono
        out2.clear();
        Builder__print_requiredBy (out2, ctx, uid, 1);
        {
            asset2::dependency_get_requireBy_list (ctx, uid, true, &listUID);
            out << "\n" << "  required by (" << listUID.getNElem() <<"): ";
            listUID.forEach ( [&out](u32 index, const UID uid) {
                out << STRFMT("%016" PRIX64 "", uid._uid) << "  ";
                return true;
            });
            out << "\n";
            out << out2;
        }        

        //lista delle "dipendenze runtime"
        if (uid.isAnAsset())
        {
            asset2::asset_get_runtime_dependecies_list (ctx, uid, true, &fastUIDList);

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
void Builder::get_dependencies_report (gos::UTF8String &out, const char *dbName, const char *baseFolder)
{
    DBContext ctx;
    bool ret;
    if (NULL == dbName)
        ret = dbcontext_open_ex (baseFolder, GOS_ASSET2__DEFAULT_DB_NAME, &ctx);
    else
        ret = dbcontext_open_ex (baseFolder, dbName, &ctx);
    if (!ret)
    {
        logger::err ("Can't open context in %s\n", baseFolder);
        return;
    }

    db::RST rstAssetList;
    db::RST rst2;

    //resource list
    {
        out << "\n\n"
            << "========================== RESOURCES LIST ==========================\n\n"
            << "Resource UID     | Type         | Abspath\n";

        db::query (ctx.db, "SELECT UID FROM " GOS_ASSET2__TABLE_RES " ORDER BY UID", &rstAssetList);
        Builder__do_print (ctx, out, rstAssetList);
    }

    //virtual asset list
    {
        out << "\n\n"
            << "========================== VIRTUAL ASSETS LIST ==========================\n\n"
            << "Asset UID        | Type         | Declared in                | runtimeName-list\n";

        db::query (ctx.db, "SELECT UID FROM " GOS_ASSET2__TABLE_VIRTUAL_ASSET " ORDER BY UID", &rstAssetList);
        Builder__do_print (ctx, out, rstAssetList);
    }

    //asset list
    {
        out << "\n\n"
            << "========================== ASSETS LIST ==========================\n\n"
            << "Asset UID        | Type         | runtimeName-list\n"
            << "-----------------+--------------+--------------------------------------------------------------\n";

        db::query (ctx.db, "SELECT UID FROM " GOS_ASSET2__TABLE_ASSET_LIST " ORDER BY UID", &rstAssetList);
        while (rstAssetList.fetchRow())
        {
            UID uid;  uid._uid = rstAssetList.getValAsU64(0);

            out << STRFMT("%016" PRIX64 "", uid._uid) << " | " 
                << STRFMT("%-12s", asset2::enumToString (uid.getAssetType())) << " | ";

            char s[512];
            sprintf_s (s, sizeof(s), "SELECT rtname FROM " GOS_ASSET2__TABLE_VIRTUAL_ASSET " WHERE UID_asset=%" PRIu64 " ORDER BY rtname", uid._uid);
            db::query (ctx.db, s, &rst2);
            while (rst2.fetchRow())
            {
                const char *name = rst2.getVal(0);
                if (name[0] != '_' && name[1] != '_')
                    out << "\"" << name << "\" ";
            }
            out << "\n";
            
            sprintf_s (s, sizeof(s), "SELECT childUID FROM " GOS_ASSET2__TABLE_DEPENDS_RUNTIME " WHERE UID=%" PRIu64 " ORDER BY childUID", uid._uid);
            db::query (ctx.db, s, &rst2);
            if (rst2.fetchRow())
            {
                out << "\truntime-dep: " << STRFMT("%016" PRIX64 "", rst2.getValAsU64(0));
                while (rst2.fetchRow())
                {
                    out << " | " << STRFMT("%016" PRIX64 "", rst2.getValAsU64(0));
                }
                out << "\n";
            }
        }
    }

    dbcontext_close(ctx);
}

//***********************************
void Builder::print_dependencies_report (const char *baseFolder, const char *dbName)
{
    gos::UTF8String out;
    out.prealloc (4096);

    get_dependencies_report (out, dbName, baseFolder);
    printf ("%s\n\n", out.getBuffer());
}

//***********************************
void Builder::save_dependencies_report (const char *baseFolder, const char *dbName)
{
    gos::UTF8String out;
    out.prealloc (4096);
    get_dependencies_report (out, dbName, baseFolder);

    
    char s[512];
    if (NULL == dbName)
        sprintf_s (s, sizeof(s), "%s/asset_bin/__dependencies.txt", baseFolder);
    else
        sprintf_s (s, sizeof(s), "%s/asset_bin/__dependencies_%s.txt", baseFolder, dbName);
    fs::fileSaveBuffer (s, out.getBuffer(), out.lengthInByte());
}
