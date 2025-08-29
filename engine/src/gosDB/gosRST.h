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
            void        rewind();

            u32         getNumCols() const          { return numCols; }
            const char* getColName (u32 colIndex);

            const char* getVal (u32 index)  const;
            u8          getValAsU8 (u32 index)  const;
            i8          getValAsI8 (u32 index)  const;
            u16         getValAsU16 (u32 index)  const;
            i16         getValAsI16 (u32 index)  const;
            u32         getValAsU32 (u32 index)  const;
            i32         getValAsI32 (u32 index)  const;
            u64         getValAsU64 (u32 index)  const;
            i64         getValAsI64 (u32 index)  const;

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