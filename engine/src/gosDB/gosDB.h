#ifndef _gosDB_h_
#define _gosDB_h_
#include "gosDBEnumAndDefine.h"
#include "gosRST.h"

namespace gos
{
    namespace db
    {
        bool    open (const char *dbfile, DBHandle *out);
        void    close (DBHandle &h);

        bool    exec (DBHandle &h, const char *sql);

        bool    query (DBHandle &h, const char *sql, RST *out);

        bool    transaction_begin (DBHandle &h);
        bool    transaction_commit (DBHandle &h);
        bool    transaction_rollback (DBHandle &h);

    } //namespace res
} //namespace gos

#endif //_gosDB_h_