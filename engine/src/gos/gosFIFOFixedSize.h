#ifndef _gosFIFOFixedSize_h_
#define _gosFIFOFixedSize_h_
#include "memory/gosMemory.h"



namespace gos
{
	/*========================================================
	* FIFOFixedSize
	*
	* FIFOFixedSize non thread safe
	* La coda puo' contenere un max di N elementi di tipo T dopodich� si ricicla a partire dal primo elemento
	*/
	template <class T, int N>
	class FIFOFixedSize
	{
	public:
                            FIFOFixedSize ()						{ reset(); }

		void				reset()									{ iPush = iPop = 0; bFull = 0; }

		bool				isEmpty() const							{ return (!bFull && (iPush == iPop)); }

		void				push (const T &val)
							{
								if (bFull)
								{
									assert (iPush == iPop);
									blob[iPush++] = val;
									iPop++;
									if (iPush == N)
									{
										iPush = 0;
										iPop = 0;
									}
								}
								else
								{
									assert (isEmpty() || (!isEmpty() && iPush != iPop) );
									blob[iPush++] = val;
									if (iPush == N)
										iPush = 0;

									if (iPush == iPop)
										bFull = 1;
								}
							}

		bool				pop (T &t)
							{
								if (isEmpty())
									return false;
								t = blob[iPop++];
								if (N == iPop)
									iPop = 0;
								bFull = 0;
								return true;
							}

		bool				top (T &t) const
							{
								if (isEmpty())
									return false;
								t = blob[iPop];
								return true;
							}


	private:
		u16					iPush;	//indice dove viene inserito il push
		u16					iPop;	//indice da dove viene fatto il pop
		u8					bFull;
		T					blob[N];
	};
} //namespace gos
#endif //_gosFIFOFixedSize_h_

