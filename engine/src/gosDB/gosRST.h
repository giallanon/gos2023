#ifndef _gosRST_h_
#define _gosRST_h_
#include "gosDBEnumAndDefine.h"
#include "gos.h"

namespace gos
{
    namespace db
    {
        /**
         * @brief   RST
         *          recordset ritornato da db::query()
         */     
        class RST
        {
        public:
                        RST();
                        ~RST()                      { priv_free(); }

            bool        fetchRow();

            u32         getNumCols() const          { return numCols; }

            const char* getColValue (u32 index)  const;
            u32         getColValueAsU32 (u32 index)  const;
            i32         getColValueAsI32 (u32 index)  const;
            u64         getColValueAsU64 (u32 index)  const;
            i64         getColValueAsI64 (u32 index)  const;

        private:
            static char strNULL[8];

        private:
            void    priv_free();
            bool    priv_query (DBHandle &h, const char *sql);

        private:
            sqlite3_stmt    *stmt;
            u32             numCols;

        friend bool query (DBHandle &h, const char *sql, RST *out);
        }; //classt RST

    } //namespace res
} //namespace gos


#endif //_gosRST_h_