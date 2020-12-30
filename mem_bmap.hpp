#ifndef __MEM_BMAP_HPP__
#define __MEM_BMAP_HPP__


//	get size_t and caddr_t
#include <sys/types.h>

struct MemBitmap
{
	unsigned long		d_numberOfBits;
	unsigned long		d_filled;
	unsigned long		d_bits[1];

};

// Create a new bitmap
MemBitmap*		MemBitmap_create( size_t a_size );

// Destroy a bitmap
void			MemBitmap_destroy( MemBitmap* a_bitmapToDesroy );

// Look for a block inside a Cluster managed by this bitmap
unsigned long	MemBitmap_findBlock( MemBitmap* a_whereToLook );

// Mark a block managed by this bitmap as used by setting it to 1
void			MemBitmap_mark( MemBitmap*	a_whereToMark,
								unsigned long a_whichBit );
// Mark a block managed by this bitmap as free by setting it to 0
void			MemBitmap_unmark( MemBitmap*	a_whereToUnmark,
								  unsigned long	a_whichBit );

// Mark all blocks managed by this bitmap as free by setting them to 0
void			MemBitmap_clear( MemBitmap*		a_whereToClear );

#endif // __MEM_BMAP_HPP__
