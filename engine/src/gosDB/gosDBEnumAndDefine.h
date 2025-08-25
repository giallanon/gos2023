#ifndef _gosDBEnumAndDefine_h_
#define _gosDBEnumAndDefine_h_
#include "gosEnumAndDefine.h"
#include "sqlite-3.50.4/sqlite3.h"


namespace gos
{
    struct DBHandle
    {
    public:
        DBHandle()          { db = NULL; }

    public:
        sqlite3 *db;
    };

} //namespace gos


#endif //_gosDBEnumAndDefine_h_