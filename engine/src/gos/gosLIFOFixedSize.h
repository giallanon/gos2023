#ifndef _gosLIFOFixedSize_h_
#define _gosLIFOFixedSize_h_
#include "memory/gosMemory.h"

namespace gos
{
	/*********************************************************************
	* LIFOFixedSize
	*
	* LIFOFixedSize non thread safe
	* La coda puo' contenere un max di N elementi di tipo T
	*/
	template <class T, int N>
	class LIFOFixedSize
	{
	public:
							LIFOFixedSize ()								{ nElem = 0; }

		u32					getNumElem() const								{ return nElem; }

		void				reset()											{ nElem=0; }

		bool				push (const T &val)
							{
								assert (nElem < N-1);
								if (nElem >= N)
									return false;
								blob[nElem++] = val;
								return true;
							}

		bool				pop (T &t)
							{
								if (nElem == 0)
									return false;
								t = blob[--nElem];
								return true;
							}

		bool				top (T &t) const
							{
								if (nElem == 0)
									return false;
								t = blob[nElem-1];
								return true;
							}
	private:
		u32					nElem;
		T					blob[N];
	};
} //namespace gos
#endif //_gosLIFOFixedSize_h_

