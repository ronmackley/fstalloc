
#ifndef		__MEM_BMAP_HPP__
#include	"mem_bmap.hpp"
#endif		// __MEM_CLST_HPP__

#ifndef		__MEM_CLST_HPP__
#include	"mem_clst.hpp"
#endif		// __MEM_CLST_HPP__

#include	<limits.h>
#include	<stddef.h>
#include	<stdio.h>

extern size_t				s_clusterSize;

namespace
{
	// since our view of the space for bitmaps is an
	// array of unsigned longs, make it be one.
	// This is really just a cluster
	MemBitmap**			s_bitmapRoot = NULL;

}
#define HIGH_ORDER_BIT				0x80000000

//****************************************************************************
//
//	MemBitmap_create() - Create a new bitmap by eeking it out of the block
//
//	ARGUMENTS:
//		a_blockSize - the size of blocks managed by this bitmap
//
//
//	RETURNS:
//		a pointer to the initialized bitmap
//		NULL if a bitmap could not be allocated
//
//****************************************************************************
MemBitmap*		MemBitmap_create( size_t a_blockSize )
{
	// initialize the bitmapRoot, if it's not alread done
	if( s_bitmapRoot == NULL )
	{
		// Allocate a cluster to hold all bitmaps
		s_bitmapRoot = (MemBitmap**)Cluster_request();
		if( s_bitmapRoot == NULL )
		{
			return NULL;
		}
	}

	// go from number of bytes in a single allocation block
	// to number of words the bitmap needs.
	long			numberOfBits =  s_clusterSize / a_blockSize ;
	long			numberOfWords = numberOfBits >> 5 ;
	// If it takes fewer bits than 1 word to hold the bits for this bitmap
	// the calculation above will report 0. Adjust accordingly.
	if( numberOfWords == 0 )
	{
		numberOfWords = 1;
	}
	
	// Add one for the the word to hold the number of words
	// and one to hold the status
	numberOfWords+=2;

	// The value we are going to return
	MemBitmap*		foundBitmap;
	
	// look in the bitmapRoot for a hunk of memory to use
	// which bitmapBlock in bitmapRoot are we looking it
	long			bitmapBlock;		  

	// loop counter variable, mark position in the bitmapBlock
	long			index;

	// sentry variable to determine when we've found a spot big enough
	long			freeZoneSize = 0;

	// Can this be optimized with a bit shift?
	long wordsPerCluster = s_clusterSize >> 5;
		
	for( bitmapBlock = 0;
		 bitmapBlock < s_clusterSize >> 2;
		 bitmapBlock++ )
	{

		// create a bitmapBlock if it is not there.
		// We will always exit the loop if we had
		// to create a bitmapBlockn
		if( s_bitmapRoot[bitmapBlock] == NULL )
		{
			s_bitmapRoot[bitmapBlock] = (MemBitmap*)Cluster_request();
			if( s_bitmapRoot[bitmapBlock] == NULL )
			{
				return NULL;
			}
		}
		// view the bitmap as an array of longs
		unsigned long*		bitmapView =
								(unsigned long*)s_bitmapRoot[bitmapBlock];

		for( index = 0, freeZoneSize = 0;
			 index < wordsPerCluster;
			 index++ )
		{
			// look for the start of a free hunk. The d_numberOfBits 
			// value is non-zero for an active bitmap
			if( bitmapView[index] == 0L )
			{
				// Don't get your hopes up. If there are not enough
				// words left to hold our bitmap, stop and
				// go to the next block.
				//  ( bits per cluster) < ( bits required )
				if( wordsPerCluster < index + numberOfWords )
				{
					// Break out and go to the next bitmap block
					break;
				}
				// mark that we are in a candidate for our bitmap
				// and keep track of how much we've gathered
				freeZoneSize++;
			}
			// We've stumbled onto an existing bitmap
			else
			{
				// Skip ahead the size of that bitmap
				index += bitmapView[index];
			}


			//We've stepped ahead, now see if we can exit
			if( freeZoneSize == numberOfWords )
			{
				// we've found enough space. Set the start of the bitmap
				// back to the start of our hunk and break out
				//Back up one
				freeZoneSize--; 
				index -= freeZoneSize;
				foundBitmap = (MemBitmap*)&bitmapView[index];
				// Break out of these loops
				goto foundBitmap;
			}
		}
	}
	
 foundBitmap:
	// if we have reached the end of our block without finding
	// room for our new bitmap, barf
	if( freeZoneSize == 0 )
	{
		return NULL;
	}

	// set the first word of the bitmap to the number of bits
	// in this bitmap
	foundBitmap->d_numberOfBits = numberOfBits;
	foundBitmap->d_filled = 0;
	// and return its head.
	return foundBitmap;
}

//****************************************************************************
//
//	MemBitmap_destroy() - Destroy a bitmap
//
//	ARGUMENTS:
//		a_bitmapToDestroy			- bitmap to be zeroed
//
//****************************************************************************
void			MemBitmap_destroy( MemBitmap* a_bitmapToDestroy )
{
	// zero all bits for this bitmap
	MemBitmap_clear( a_bitmapToDestroy );

    // zero the number of bits field for this bitmap
	a_bitmapToDestroy->d_numberOfBits = 0L;
}

//****************************************************************************
//
//	MemBitmap_findBlock() - Look for a block inside a Cluster
//							managed by this bitmap
//
//	ARGUMENTS:
//		a_whereToLook			- bitmap to find a block in
//
//	RETURNS:
//		index of available free block
//		ULONG_MAX if a block cannot be found
//
//****************************************************************************
unsigned long			MemBitmap_findBlock( MemBitmap* a_whereToLook )
{
	// Iterate over each bit using bitIndex as a base counter,
	// using mods of word size on bitINdex fiure out when
	// we need to increment wordIndex to the next word

	// If this bitmap is filled, don't bother
	if( a_whereToLook->d_filled == 1 )
	{
		return ULONG_MAX;
	}
  
	// Set a starting point for the bitmap
	unsigned long*		bitMap = a_whereToLook->d_bits;

	unsigned long index = 0UL;
	unsigned long bitMask = HIGH_ORDER_BIT;
	unsigned long maxCount = a_whereToLook->d_numberOfBits;

	if( maxCount < 32 )
	{
		// Search on a bit by bit basis if there are fewer than
		// word-size number of bits in this bitmap.
		while( index < maxCount )
		{
			// Check to see if the bit is free.
			if( !(*bitMap & bitMask) )
			{
				// If it is, return it's position
				return index;
			}
			// Shift toward the LOB 1 bit.
			bitMask >>= 1;
			// And try the next bit
			++index;
		}
	}
	else
	{
		// look word by word
		// divide by the word size to get the number of words to examine
		maxCount >>= 5;
		while( index < maxCount )
		{
			// shortCircuit in case no bit is free
			if( (*bitMap & 0xffffffff) == 0xffffffff )
			{
				bitMask = 0UL;
				++index;
				continue;
			}

			// We've established that a bit is free, now find it
			// Set or reset a bit mask starting with the HOB
			// This will be shifted toward the LOB
			bitMask = HIGH_ORDER_BIT;

			while( bitMask )
			{
				// Check to see if the bit is free.
				if( !(*bitMap & bitMask) )
				{
					// If it is, return it's position
					return index;
				}
				// Shift toward the LOB 1 bit.
				bitMask >>= 1;
			}
		}
	}
	// Could not find a bit
	a_whereToLook->d_filled = 1;
	return ULONG_MAX;
}

//****************************************************************************
//
//	MemBItmap_mark() - Mark a block managed by this bitmap as used by
//					   setting it to 1
//
//	ARGUMENTS:
//		a_whereToMark			- bitmap to work in
//		a_whichBit				- index of the bit to diddle
//
//****************************************************************************
void			MemBitmap_mark( MemBitmap*			a_whereToMark,
								unsigned long		a_whichBit )
{
	//Divide the which bit by 32 to get which word
	unsigned long		whichWord = a_whichBit >> 5;
	// Get the modulus of 32 to get which bit of which word
	unsigned long		whichBit  = a_whichBit & 0x0000001f;
	// shift the mask into place
	unsigned long		mask = HIGH_ORDER_BIT;
	mask >>= whichBit;

	// and mark it
	a_whereToMark->d_bits[whichWord] |= mask;
}

//****************************************************************************
//
//	MemBitmap_unmark() - Mark a block managed by this bitmap as unused by
//						 setting it to 0
//
//	ARGUMENTS:
//		a_whereToUnmark			- bitmap to work in
//		a_whichBit				- index of the bit to diddle
//
//****************************************************************************
void			MemBitmap_unmark( MemBitmap*	a_whereToUnmark,
								  unsigned long			a_whichBit )
{
	//Divide the which bit by 32 to get which word
	unsigned long		whichWord = a_whichBit >> 5;
	// Get the modulus of 32 to get which bit of which word
	unsigned long		whichBit  = a_whichBit & 0x0000001f;
	// shift the mask into place
	unsigned long		mask = HIGH_ORDER_BIT;
	mask >>= whichBit;

	// and unmark it
	a_whereToUnmark->d_bits[whichWord] &= ~mask;
	a_whereToUnmark->d_filled = 0;
}

//****************************************************************************
//
//	MemBitmap_clear() - Mark all blocks managed by this bitmap
//						as free by setting them to 0
//
//	ARGUMENTS:
//		a_whereToClear			- bitmap to clear
//
//****************************************************************************

void			MemBitmap_clear( MemBitmap*		a_whereToClear )
{
	unsigned long	numberOfWords = 1 + (a_whereToClear->d_numberOfBits >> 5);
	
	// fill the bitmap with 0's, leave d_numberOfBits alone
	for( unsigned long i = 0; i < numberOfWords; i++ )
	{
		a_whereToClear->d_bits[i] = 0L;
	}
}
