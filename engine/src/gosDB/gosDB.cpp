#include "gosDB.h"
#include "gos.h"

using namespace gos;


//***************************************************
bool db::open (const char *dbfile, DBHandle *out)
{
    assert (NULL != out);
    
    if (SQLITE_OK == sqlite3_open (dbfile, &out->db))
        return true;

    logger::err ("db::open => can't open database '%s', error: %s", dbfile, sqlite3_errmsg(out->db));
    close (*out);
    return false;
}

//***************************************************
void db::close (DBHandle &h)
{
    if (NULL == h.db)
        return;

    sqlite3_close(h.db);
    h.db = NULL;
}

//***************************************************
bool db::exec (DBHandle &h, const char *sql)
{
    if (NULL == h.db)
        return false;

    int rc;

    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2 (h.db, sql, -1, &stmt, NULL);
    if (SQLITE_OK != rc)
    {
        logger::err ("db::exec => error '%s' when executing '%s'\n", sqlite3_errstr(rc), sql);
        assert (NULL == stmt);
        return false;
    }

    if (NULL == stmt)
        return true;

    char *errMsg = NULL;
    rc = sqlite3_exec(h.db, sql, NULL, 0, &errMsg);
    if (SQLITE_OK != rc)
    {
        logger::err ("db::exec => error '%s' when executing '%s'\n", sqlite3_errstr(rc), sql);
        sqlite3_finalize (stmt);
        return false;
    }

    sqlite3_finalize (stmt);
    return true;
}

//***************************************************
bool db::query (DBHandle &h, const char *sql, RST *out )
{
    if (NULL == h.db)
        return false;

    if (NULL == out)
        return false;

    return out->priv_query (h, sql);
}

//***************************************************
bool db::transaction_begin (DBHandle &h)
{
    return db::exec (h, "BEGIN TRANSACTION");
}

//***************************************************
bool db::transaction_commit (DBHandle &h)
{
    return db::exec (h, "COMMIT");
}

//***************************************************
bool db::transaction_rollback (DBHandle &h)
{
    return db::exec (h, "ROLLBACK");
}
