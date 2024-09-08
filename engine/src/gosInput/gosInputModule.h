#ifndef _gosInputModule_h_
#define _gosInputModule_h_
#include "gosInputEnumAndDefine.h"
#include "gosInputEvtList.h"
#include "gosInputWindow.h"
#include "gosInputContext.h"

namespace gos
{
	namespace input
	{
        class Module
        {
        public:
							Module (gos::Allocator *allocator);
							~Module();

			//============== utils
			Allocator*		getAllocator() const 								{ return localAllocator; }

        public:
            gos::HandleList<GOSWinHandle, input::Window*>   windowList;
            input::EvtList                                  voidEvtList;

		private:
			gos::Allocator 				*localAllocator;

        };		
	} //namespace input
} //namespace gos
#endif //_gosInputModule_h_