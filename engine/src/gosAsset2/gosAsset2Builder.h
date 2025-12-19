#ifndef _gosAsset2Builder_h_
#define _gosAsset2Builder_h_
#include "gosAsset2EnumAndDefine.h"
#include "gosIniFile.h"

namespace gos
{
	namespace asset2
	{
		/**************************************************
		* Builder
		* 
		*/
		class Builder
		{
		public:
						Builder();
						~Builder();

			bool		rebuildAll (const char *baseFolder);
			bool		build (const char *baseFolder);


        private:
            enum class eBuildStatus : u8
            {
                DONT_KNOW   = 0,
                NEW         = 1,
                MODIFIED    = 2,
                DELETED     = 3,
                UNCHANGED   = 4
            };

        private:
            struct sResListElem
            {
            public:
                void        reset() { uid._uid=0; abspath[0]=0x00; lastTimeModified=0; resType=eResType::__FINISHED; status=eBuildStatus::DONT_KNOW; }

            public:
                UID             uid;
                char            abspath[512];
                u64             lastTimeModified;
                eResType        resType;
                eBuildStatus    status;
            };

        private:
            typedef gos::FastArray<sResListElem>    ResList;

		private:
            void		priv_gosassetd_scan_DB (DBContext &ctx, ResList *out_list) const;
			bool		priv_gosassetd_scan_folder (DBContext &ctx, const char *folder_path, ResList *out_list) const;
            bool        priv_gosassetd_scan_folder_parse (DBContext &ctx, const char *filename, ResList *out_list) const;

		private:
			gos::Allocator	*localAllocator;
		};

	} //namespace asset2
} //namespace gos


#endif //_gosAsset2Builder_h_

