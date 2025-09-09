#ifndef _gosFIFOFixedSize_h_
#define _gosFIFOFixedSize_h_
#include "memory/gosMemory.h"



namespace gos
{
	/*========================================================
	* FIFOFixedSize
	*
	* FIFOFixedSize non thread safe
	* La coda puo' contenere un max di N elementi di tipo T dopodiche' si ricicla a partire dal primo elemento
	*/
	template <class T, int N>
	class FIFOFixedSize
	{
	public:
                            FIFOFixedSize ()						{ reset(); }

		void				reset()									{ iPush = iPop = 0; numElem = 0; }

		bool				isEmpty() const							{ return (numElem == 0); }

		void				push (const T &val)
							{
								if (N == numElem)
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

									numElem++;
#ifdef _DEBUG
									if (iPush == iPop)
									{
										assert (N == numElem);
									}
#endif
								}
							}

		bool				pop (T *out_val)
							{
								assert (NULL != out_val);
								if (isEmpty())
									return false;
								*out_val = blob[iPop++];
								if (N == iPop)
									iPop = 0;
								
								numElem--;
								return true;
							}

		bool				top (T *out_val) const
							{
								if (isEmpty())
									return false;
								*out_val = blob[iPop];
								return true;
							}

		u32 				getNElem() const 						{ return numElem; }							

	private:
		u16					iPush;	//indice dove viene inserito il push
		u16					iPop;	//indice da dove viene fatto il pop
		u32					numElem;
		T					blob[N];
	};
} //namespace gos
#endif //_gosFIFOFixedSize_h_

