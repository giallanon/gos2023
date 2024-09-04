#ifndef _gosInputResolvedEvtList_h_
#define _gosInputResolvedEvtList_h_
#include "gosInputEnumAndDefine.h"
#include "gosInputEvtList.h"

namespace gos
{
	namespace input
	{
		struct Window; //fwd
		class Context; //fwd

		/**
		 * @brief ResolvedEvtList
		 */
		class ResolvedEvtList
		{
		public:
										ResolvedEvtList();
			
			u32							nextActionID (i16 *out_value);

			const MouseStatus&			getMouseStatus() const;
			const sButtonModifier&		getBtnModifier() const;

		private:
			void 						setup (Window *win);

		private:
			Window 						*win;


		friend void resolveEvents (const GOSWinHandle &handle, const Context *ctx, ResolvedEvtList *out);
		};
		
	} //namespace input
} //namespace gos
#endif //_gosInputResolvedEvtList_h_