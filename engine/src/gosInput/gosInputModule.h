#ifndef _gosInputModule_h_
#define _gosInputModule_h_
#include "gosInputEnumAndDefine.h"
#include "gosInputEvtList.h"
#include "gosInputWindow.h"
#include "gosInputContext.h"
#include "../gos/gosSortedFastArray.h"

namespace gos
{
	namespace input
	{
        class Module
        {
        public:
							Module (gos::Allocator *allocator);
							~Module();


			u32				action_addName (const char *name);
			const char*		action_getNameByOffset (u32 offset) const			{ return actionNameList.getStringAtOffset (offset); }

			//============== gestione dei context
			Context*		context_create (const char *name);
			u32				context_getNum () const								{ return contextList.getNElem(); }
			const char*		context_getName (u32 iCtx) const;
			u32				context_getNumAction (u32 iCtx) const;
			const char*		context_getActionNameByIndex (u32 iCtx, u32 iAction) const;
			u32				context_getActionIDByIndex (u32 iCtx, u32 iAction) const;

			//============== utils
			void 			logAllMappedInput() const;

        public:
            gos::HandleList<GOSWinHandle, input::Window*>   windowList;
            input::EvtList                                  voidEvtList;

		private:
			struct sActionName
			{
				u32 	nameCRC32;
				u32		stringOffset;
			};

		private:
			gos::Allocator 				*localAllocator;
			StringList					actionNameList;
			FastArray<Context*>			contextList;
			SortedFastArray<u32, u32>	hashListOfActionNames;

        };		
	} //namespace input
} //namespace gos
#endif //_gosInputModule_h_