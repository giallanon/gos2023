#ifndef _gosBit_h_
#define _gosBit_h_
#include "gos.h"

namespace gos
{
	/**
	 * @brief Manipolazione di BIT 
	 * Settano/resettano il bit in posizione [pos].
	 * [pos] == 0  => LSBit
	*/        
	void    bitZERO  (void *dst, u32 sizeof_dst);
	void    bitSET (void *dst, u32 sizeof_dst, u32 pos);
	void    bitCLEAR (void *dst, u32 sizeof_dst, u32 pos);
	bool    isBitSET (const void *src, u32 sizeof_dst, u32 pos);

	//spèecializzazione per un singolo u32
	void    bit32SET (u32 *dst, u32 pos);
	void    bit32CLEAR (u32 *dst, u32 pos);
	bool    isBit32SET (u32 src, u32 pos);

	/**
	 * @brief Manipolazione di BYTE 
	 * setta/resetta il byte posizione [pos].
	 * [pos] == 0  => LSB
	*/        
	void    byte32SET (u32 *dst, u8 value, u32 pos);
	u8      byte32GET (u32 src, u32 pos);    

	/**
	 * @brief alloca un buffer di almeno [numBit*8] byte e lo tratta
	 * come un bitfield
	 */
	class Bitfield
	{
	public:
				Bitfield()								{ p = NULL; numU64Allocati = 0; }

		void 	setup (gos::Allocator *allocator, u32 numBit);
		void 	unsetup (gos::Allocator *allocator);

		void 	zero();
		void 	set (u32 pos);
		void 	clear (u32 pos);
		bool 	isBitSet (u32 pos) const;

				/**
				 * @brief ritorna true se trova 1 bit libero, nel qual caso lo setta a 1 e mette in [out_pos] la posizione
				 * utilizzata
				 * Ritorna false se il bitfield e' pieno
				 */
		bool 	findAndSetFirstFreeBit (u32 *out_pos) const;

				/**
				 * @brief scanna il bitfield a partire da [startBit] incluso e ritorna true se trova
				 * un bit libero (nel qual caso mette in [out_pos] l'indice del bit libero.
				 * Ritorna false altrimenti
				 */
		bool 	findFirstFreeBit (u32 startBit, u32 *out_pos) const;

				/**
				 * @brief scanna il bitfield a partire da [startBit] incluso e ritorna true se trova
				 * un bit "set" (nel qual caso mette in [out_pos] l'indice del bit.
				 * Ritorna false altrimenti
				 */
		bool 	findFirstSetBit (u32 startBit, u32 *out_pos) const;

		u32 	getNumMaxBit() const					{ return numBit; }
		const u64*	getBuffer() const 					{ return p; }
	private:
		u64 	*p;
		u32		numBit;
		u32 	numU64Allocati;
	};


	/**
	 * @brief un U64 usato a mo' di bitflag
	 */
	struct Flag64
	{
	public:
		void 	zero()									{ flag = 0; }
		void 	setAll()								{ flag = u64MAX; }
		void 	set (u32 pos)							{ assert (pos < 64); flag |= ((u64)1 << pos); }
		void 	clear (u32 pos)                       	{ assert (pos < 64); flag &= ~(((u64)1 << pos)); }
		bool 	isBitSet (u32 pos) const				{ assert (pos < 64); return ((flag & ((u64)1 << pos)) != 0); }
		bool 	isZero() const							{ return (flag==0); }

		bool	operator== (const Flag64& b) const		{ return flag == b.flag; }
		bool	operator!= (const Flag64& b) const		{ return flag != b.flag; }

		u64		getBitmask() const						{ return flag; }
		void	setBitmask(u64 bitmask)					{ flag = bitmask; }

	private:
		u64 	flag;
	};

	/**
	 * @brief un U32 usato a mo' di bitflag
	 */
	struct Flag32
	{
	public:
		void 	zero()									{ flag = 0; }
		void 	setAll()								{ flag = u32MAX; }
		void 	set (u32 pos)							{ assert (pos < 32); flag |= (0x00000001 << pos); }
		void 	clear (u32 pos)                       	{ assert (pos < 32); flag &= ~((0x00000001 << pos)); }
		bool 	isBitSet (u32 pos) const				{ assert (pos < 32); return ((flag & (0x00000001 << pos)) != 0); }
		bool 	isZero() const							{ return (flag==0); }

		bool	operator== (const Flag32& b) const		{ return flag == b.flag; }
		bool	operator!= (const Flag32& b) const		{ return flag != b.flag; }

		u32		getBitmask() const 						{ return flag; }
		void	setBitmask(u32 bitmask)					{ flag = bitmask; }

	private:
		u32 	flag;
	};



	/**
	 * @brief un U16 usato a mo' di bitflag
	 */
	struct Flag16
	{
	public:
		void 	zero()									{ flag = 0; }
		void 	setAll()								{ flag = u16MAX; }
		void 	set (u32 pos)							{ assert (pos < 16); flag |= (0x00001 << pos); }
		void 	clear (u32 pos)                       	{ assert (pos < 16); flag &= ~((0x00001 << pos)); }
		bool 	isBitSet (u32 pos) const				{ assert (pos < 16); return ((flag & (0x00001 << pos)) != 0); }
		bool 	isZero() const							{ return (flag==0); }

		bool	operator== (const Flag16& b) const		{ return flag == b.flag; }
		bool	operator!= (const Flag16& b) const		{ return flag != b.flag; }

		u16		getBitmask() const						{ return flag; }
		void	setBitmask(u16 bitmask)					{ flag = bitmask; }

	private:
		u16 	flag;
	};



	/**
	 * @brief un U8 usato a mo' di bitflag
	 */
	struct Flag8
	{
	public:
		void 	zero()									{ flag = 0; }
		void 	setAll()								{ flag = 0xff; }
		void 	set (u32 pos)							{ assert (pos < 8); flag |= (0x01 << pos); }
		void 	clear (u32 pos)                       	{ assert (pos < 8); flag &= ~((0x01 << pos)); }
		bool 	isBitSet (u32 pos) const				{ assert (pos < 8); return ((flag & (0x01 << pos)) != 0); }
		bool 	isZero() const							{ return (flag==0); }

		void	operator|= (const Flag8& b)				{ flag |= b.flag; }
		void 	operator&= (const Flag8& b)				{ flag &= b.flag; }

		bool	operator== (const Flag8& b) const		{ return flag == b.flag; }
		bool	operator!= (const Flag8& b) const		{ return flag != b.flag; }
		
		u8 		getBitmask() const 						{ return flag; }
		void	setBitmask(u8 bitmask)					{ flag = bitmask; }

	private:
		u8 		flag;
	};				

       
} //namespace gos

#endif //_gosBit_h_