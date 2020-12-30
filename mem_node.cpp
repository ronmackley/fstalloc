#ifndef			__MEM_NODE_HPP__
#include		"mem_node.hpp"
#endif			// __MEM_NODE_HPP__

#ifndef			__MEM_CLST_HPP__
#include		"mem_clst.hpp"
#endif			// __MEM_CLST_HPP__

#ifndef			__MEM_BMAP_HPP__
#include		"mem_bmap.hpp"
#endif			// __MEM_BMAP_HPP__

#include		<limits.h>
#include		<stddef.h>
#include		<stdio.h>

extern			size_t s_clusterSize;

namespace
{
	// file local function to initialize necessary data structures
	void			initializeMasterNodeTable();

	// file local data structure to manage nodes
	MemNode**		s_masterNodeTable = NULL;
}



//****************************************************************************
//
//	MemNode_create - find and initialize a new node
//
//	ARGS:
//		a_size - the power of two of the block
//
//	RETURNS:
//		pointer to a new MemNode
//		NULL on error.
//
//	NOTE:
//		This function causes a new BittMap to be created and
//		a new Cluster to be requested. The intention is that
//		all allocations in this node be taken from this cluster
//		and managed using this bitmap
//
//****************************************************************************
MemNode*		MemNode_create( size_t a_size )
{
	// initialize the necessary data structures,
	// if they are not already done
	if( s_masterNodeTable == NULL )
	{
		// allocate a Cluster to hold the table of
		// MemNode pointers
		s_masterNodeTable = (MemNode**)Cluster_request();
		if( s_masterNodeTable == NULL )
		{
			// If we cannot get a cluster for the masterNodeTable
			// we're stuck.
			return NULL;
		}
	}

	// Needed outside of the loops
	MemNode*	  newNodePtr;
	
	// find the first available node in the array
	// Iterate over each node handle in the s_masterNodeTable
	for( MemNode** newNodeHandle = s_masterNodeTable;
		 (caddr_t)newNodeHandle < (caddr_t)s_masterNodeTable+s_clusterSize;
		 newNodeHandle++ )
	{
		// See if we need to make a new block to hold nodes
		if( *newNodeHandle == NULL )
		{
			// We've found a slot that needs a new cluster block	
			*newNodeHandle = (MemNode*)Cluster_request();
			if( *newNodeHandle == NULL )
			{
				// If we cannot get a cluster to hold nodes we're stuck.
				return NULL;
			}
		}
		// We have a cluster to look in, now search it for a free node
		for( newNodePtr = *newNodeHandle;
			 (caddr_t)newNodePtr < (caddr_t)*newNodeHandle + s_clusterSize;
			 newNodePtr++ )
		{
			if( newNodePtr->d_size == 0 )
			{
				// Ha! we've found our node!
				// break out of these loops
				goto foundNode;
			}
		}
	}
 foundNode:
	// Now that we have a node, initialize it
	newNodePtr->d_bitMap = MemBitmap_create( 1 << a_size );
	//newNodePtr->d_cluster = Cluster_request();
	newNodePtr->d_cluster = NULL;
	newNodePtr->d_size = a_size;
	newNodePtr->d_count = 0L;
	newNodePtr->d_previousNode = newNodePtr->d_nextNode = NULL;

	// Return the new node to the caller
	return newNodePtr;
}

//****************************************************************************
//
//	MemNode_destroy - free the resources used by this node
//
//	ARGS:
//		a_nodeToDestroy - the node to destroy
//
//	NOTE:
//		This function causes the Cluster to be released and the
//		BitMap to be freed.
//
//****************************************************************************
void			MemNode_destroy( MemNode* a_nodeToDestroy )
{
	// free our bitmap
	MemBitmap_destroy( a_nodeToDestroy->d_bitMap );
	a_nodeToDestroy->d_bitMap = NULL;

	// release our cluster
	Cluster_release( a_nodeToDestroy->d_cluster );

	// another hint that this is not used
	a_nodeToDestroy->d_size = 0;

	// free further down the chain
	// NoteToSelf: find non-recursive technique
	if( a_nodeToDestroy->d_nextNode != NULL )
	{
		MemNode_destroy( a_nodeToDestroy->d_nextNode );
		a_nodeToDestroy->d_nextNode = NULL;
	}
}

//****************************************************************************
//
//	MemNode_findBlock - find a free block in the Cluster managed by
//						this block
//
//	ARGS:
//		a_whereToLook - the node where we want to find the block
//
//	RETURNS:
//		pointer to the block.
//		NULL if a block could not be found
//
//****************************************************************************
caddr_t			MemNode_findBlock( MemNode* a_whereToLook )
{
	// get the position of the free block from the bitmap
	unsigned long	offset = MemBitmap_findBlock( a_whereToLook->d_bitMap );

	// if a block cannot be found in our node, return NULL.
	// caller can go look elsewhere
	if( offset == ULONG_MAX )
	{
		return NULL;
	}

	// Mark the slot as occupied
	MemBitmap_mark( a_whereToLook->d_bitMap, offset );
	
	// calculate its offset in the cluster
	offset <<= a_whereToLook->d_size;

	// Up the count
	a_whereToLook->d_count++;

	// If we do not have a cluster yet, create one.
	if( a_whereToLook->d_cluster == NULL )
	{
		a_whereToLook->d_cluster = Cluster_request();
	}

	// add the offset to the start of the cluster to get the address
	// of the block
	return ((caddr_t)a_whereToLook->d_cluster) + offset ;
}

//****************************************************************************
//
//	MemNode_releaseBlock - release a block in the Cluster managed by
//						   this block
//
//	ARGS:
//		a_whereToLook - the node where we want to find the block
//		a_blockToRelease - address of the block to release
//
//	NOTE:
//		Note, this routine just manipulates the bitmap to indicate
//		the memory is free. 
//
//****************************************************************************
void			MemNode_releaseBlock( MemNode* a_whereToLook,
									  caddr_t a_blockToRelease )
{	
	// calculate the  offset in the bitmap from the address of the
	// block's cluster and the address to be freed
  	unsigned long		offset = a_blockToRelease - a_whereToLook->d_cluster;

	// divide by the block size to get the index of the bit
	offset >>= (unsigned long)a_whereToLook->d_size;

	// and unmark that bit.
	MemBitmap_unmark( a_whereToLook->d_bitMap, offset );

	// Reduce the count
	a_whereToLook->d_count--;

	if( a_whereToLook->d_count == 0 )
	{
		Cluster_release( a_whereToLook->d_cluster );
		a_whereToLook->d_cluster = NULL;
	}
	// That's it! We need not touch the actual memory.
}
