#include "gosDB.h"
#include "gos.h"

using namespace gos;
using namespace gos::db;

char RST::strNULL[8] = { 'N', 'U', 'L', 'L', 0 };

//***************************************************
RST::RST()
{
    stmt = NULL;
    numCols = 0;
}

//***************************************************
void RST::priv_free()
{
    if (NULL == stmt)
        return;

    sqlite3_finalize (stmt);
    stmt = NULL;

    numCols = 0;
}

//***************************************************
bool RST::priv_query (DBHandle &h, const char *sql)
{
    priv_free();
    
    int rc;

    rc = sqlite3_prepare_v2 (h.db, sql, -1, &stmt, NULL);
    if (SQLITE_OK != rc)
    {
        logger::err ("db::query => error '%s' when executing '%s'\n", sqlite3_errstr(rc), sql);
        assert (NULL == stmt);
        return false;
    }

    if (NULL == stmt)
        return true;

    char *errMsg = NULL;
    rc = sqlite3_exec(h.db, sql, NULL, 0, &errMsg);
    if (SQLITE_OK != rc)
    {
        logger::err ("db::query => error '%s' when executing '%s'\n", sqlite3_errstr(rc), sql);
        sqlite3_finalize (stmt);
        return false;
    }

    numCols = static_cast<u32> (sqlite3_column_count (stmt));
    return true;
}

//***************************************************
bool RST::fetchRow()
{
    if (NULL == stmt)
        return false;

    int rc = sqlite3_step (stmt);
    
    if (SQLITE_ROW == rc)
        return true;
    
    return false;
}

//***************************************************
const char* RST::getColValue (u32 index) const
{
    if (NULL == stmt || index >= getNumCols())
        return strNULL;

    return reinterpret_cast<const char*> (sqlite3_column_text (stmt, index));
}

//***************************************************
u8 RST::getColValueAsU8 (u32 index) const               { if (NULL == stmt || index >= getNumCols()) return 0; return static_cast<u8> (sqlite3_column_int (stmt, index)); }
i8 RST::getColValueAsI8 (u32 index) const               { if (NULL == stmt || index >= getNumCols()) return 0; return static_cast<i8> (sqlite3_column_int (stmt, index)); }
u16 RST::getColValueAsU16 (u32 index) const             { if (NULL == stmt || index >= getNumCols()) return 0; return static_cast<u16> (sqlite3_column_int (stmt, index)); }
i16 RST::getColValueAsI16 (u32 index) const             { if (NULL == stmt || index >= getNumCols()) return 0; return static_cast<i16> (sqlite3_column_int (stmt, index)); }

//***************************************************
u32 RST::getColValueAsU32 (u32 index) const
{
    if (NULL == stmt || index >= getNumCols())
        return 0;

    return static_cast<u32> (sqlite3_column_int (stmt, index));
}

//***************************************************
i32 RST::getColValueAsI32 (u32 index) const
{
    if (NULL == stmt || index >= getNumCols())
        return 0;

    return static_cast<i32> (sqlite3_column_int (stmt, index));
}

//***************************************************
u64 RST::getColValueAsU64 (u32 index) const
{
    if (NULL == stmt || index >= getNumCols())
        return 0;

    return static_cast<u64> (sqlite3_column_int64 (stmt, index));
}

//***************************************************
i64 RST::getColValueAsI64 (u32 index) const
{
    if (NULL == stmt || index >= getNumCols())
        return 0;

    return static_cast<i64> (sqlite3_column_int64 (stmt, index));
}