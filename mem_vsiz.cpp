#ifndef			__MEM_VSIZ_HPP__
#include		"mem_vsiz.hpp"
#endif			// __MEM_VSIZ_HPP__

#ifndef			__MEM_CLST_HPP__
#include		"mem_clst.hpp"
#endif			// __MEM_CLST_HPP__

#include		<stdio.h>
#include		<assert.h>

// file-static structures functions and data
namespace
{
	struct node
	{
		// d_size is size of data + HEADER_SIZE
		size_t			d_size;
		// NULL means end of the list
		struct node*	d_next;
		// this block is really an array of size bytes
		char			d_block;
	};

	// useful typedefs
	typedef struct node node;
	typedef	node*  node_ptr;

	// the size of an allocation block header
	// these numbers are:  d_prev + d_next + d_size
	const unsigned long HEADER_SIZE	 =
		  ( sizeof(node_ptr) + sizeof(node_ptr) + sizeof(size_t));

	// SMALLEST_ALLOC must always be a power of 2
	const unsigned long SMALLEST_ALLOC = 32;
	// Mask for doing mods of SMALLEST_ALLOC
	const unsigned long SMALLEST_ALLOC_MASK = SMALLEST_ALLOC-1;
}

// Make nodes pointing to self to serve as the base for the lists.
node		s_freeListHead = { 0, &s_freeListHead, '\0' };
node		s_usedListHead = { 0, &s_usedListHead, '\0' };

// heads for linked lists to manage used and free blocks
node_ptr	s_freeList = &s_freeListHead;
node_ptr	s_usedList = &s_usedListHead;

//****************************************************************************
//
//	Mem_varSizeAlloc() - allocate a node from the free list, breaking nodes
//						 into smaller pieces if necessary
//
//	PARAMETERS:
//		a_size - how big of a block to allocate.
//
//	RETURNS:
//		pointer to data member of node allocated
//
//
//****************************************************************************
caddr_t					Mem_varSizeAlloc( size_t a_size )
{
	node_ptr			currentNode;
	node_ptr			prevNode = NULL;
	node_ptr			newFreeNodePtr;

	// increase the required node size to include the header
	size_t				requestSize = a_size;
	requestSize += HEADER_SIZE;

	// and align it on a SMALLEST_ALLOC boundary
	requestSize += SMALLEST_ALLOC - ( requestSize & SMALLEST_ALLOC_MASK );
	
	// walk the list of free nodes until the first fit is found
	currentNode = s_freeList;
	while( currentNode->d_size < requestSize &&
		   currentNode->d_next != &s_freeListHead )
	{
		prevNode = currentNode;
		currentNode = currentNode->d_next;
	}
	// CurrentNode contains either a node large enough for the request
	//						or
	// The next node is the list head, indicating we've reached the end.

	// If we reach the list head, known because of the 0 size, get another
	// slab and add it to the list.
	if( currentNode->d_next == &s_freeListHead &&
		currentNode->d_size < requestSize )
	{
		// Get another slab of memory
		size_t		   size = requestSize;
		caddr_t		   newSlab = Cluster_bigRequest( &size );
		// If we could not fulfill the request, return NULL
		if( newSlab == NULL )
		{
			return NULL;
		}

		// Make the new slab into a node
		node_ptr	slabNode = (node_ptr)newSlab;
		slabNode->d_size = size;

		// Walk the list to find the proper place for this node
		currentNode = s_freeList;
		while( currentNode->d_next < slabNode &&
			   currentNode->d_next != &s_freeListHead )
		{
			currentNode = currentNode->d_next;
		}
		// current node is just before where slab node needs to be inserted or
		// we are at the end of the list.
		// In either case, insert slabNode after currentNode.
#ifdef DEBUG
		fprintf( stderr,
		"Alloc: put new slab %8.8x on free list between %8.8x and %8.8x\n",
		slabNode, currentNode, currentNode->d_next );
#endif
		slabNode->d_next = currentNode->d_next;
		currentNode->d_next = slabNode;
		currentNode = slabNode;
	}

	// we now have the first fit, break it to the size we need
	// and insert it into the used list

	// fix the list to remove the block just allocated
	if( currentNode->d_size == requestSize )
	{
		// if we have an exact fit, just remove the node entirely
		prevNode->d_next = currentNode->d_next;
		// currentNode now contains the block to fulfill the request
#ifdef DEBUG
	   	fprintf( stderr,
				 "Alloc: took %8.8x from the free list\n",
				 currentNode );
#endif
	}
	// otherwise we need to break currentNode into two pieces
	else
	{
		// Shrink the size of current node, leave the left part in the list
		// and take the right part to fulfill the request.
		currentNode->d_size -= requestSize;

		// set currentNode to point to the right half of the block being
		// broken to satisfy the request
		currentNode = (node_ptr)((caddr_t)currentNode + currentNode->d_size);
#ifdef DEBUG
	   	fprintf( stderr,
				 "Alloc: Broke %8.8x out of the free list\n",
				 currentNode );
#endif
	}
	// Current node is no longer a node in the free list

	// update the node to be returned

	// fix the size
	currentNode->d_size = requestSize;

	// Stop looking if we reach the end of the list
	// or just before the insertion spot
	node_ptr		  srchNode = s_usedList;
	prevNode = NULL;
	while( srchNode->d_next < currentNode &&
		   srchNode->d_next != &s_usedListHead )
	{
		prevNode = srchNode;
		srchNode = srchNode->d_next;
	}
#ifdef DEBUG
	fprintf( stderr,
	"Alloc: put %8.8x on used list between %8.8x and %8.8x\n",
	currentNode, srchNode, srchNode->d_next );
#endif

	// srchNode is now positioned just before currentNode
	currentNode->d_next = srchNode->d_next;
	srchNode->d_next = currentNode;

#ifdef DEBUG
   	fprintf( stderr,
			 "Alloc: returning %8.8x from node %8.8x\n",
				 &currentNode->d_block, currentNode );
#endif
	return &currentNode->d_block;
}

//****************************************************************************
//
//	Mem_varSizeFree() - return node back to the free list and join adjacent
//						nodes
//
//	PARAMETERS:
//		a_addr: the address of the block member of the node to free
//
//****************************************************************************
void					Mem_varSizeFree( caddr_t a_addr )
{
	// we can always free NULL
	if( a_addr == NULL ) return;

// 	// we can also always free if there is
// 	// nothing to be freed
// 	if( s_usedList->d_next = &s_usedListHead )
// 	{
// 		return;
// 	}

	// calculate the address of the node
	node_ptr			nodeAddr = (node_ptr)((caddr_t)a_addr - 8);
	// from here on out, any reference to size includes HEADER_SIZE

#ifdef DEBUG
	   fprintf( stderr, "Attempt to free block: %8.8x\n",
				nodeAddr );
#endif
	
	// Start at the head of the used list
	node_ptr			currentNode = s_usedList;

	while( currentNode->d_next != nodeAddr &&
		   currentNode->d_next != &s_usedListHead )
	{
		currentNode = currentNode->d_next;
	}
	// currentNode->d_next points either to the head of the list
	// or to the node to be freed.
	
	// we made it to the end without finding our block.
	if( currentNode->d_next == &s_usedListHead )
	{
#ifdef DEBUG
	   fprintf( stderr,
				"Free: attempt to free block not held: %8.8x\n",
				nodeAddr );
#endif
		return;
	}

#ifdef DEBUG
	   fprintf( stderr,
				"Free: found block: %8.8x in used list. \nIt better be the same as %8.8x\n",
				currentNode->d_next, nodeAddr );
#endif
	// remove the node to close the gap
	currentNode->d_next = nodeAddr->d_next;

	// remember, nodeAddr contains what we're freeing

	// put nodeAddr back into the free list

	// start at the head of the free list
	currentNode = s_freeList;
	node_ptr				prevNode = s_freeList;

	while( currentNode->d_next != &s_freeListHead &&
		   currentNode->d_next < nodeAddr )
	{
		prevNode = currentNode;
		currentNode = currentNode->d_next;
	}
	nodeAddr->d_next = currentNode->d_next;
	currentNode->d_next = nodeAddr;

	// then see whether we can coalesce adjacent nodes
	// first with the next node in the list
	if( ((caddr_t)nodeAddr)+nodeAddr->d_size == (caddr_t)nodeAddr->d_next )
	{
		nodeAddr->d_size += nodeAddr->d_next->d_size;
		nodeAddr->d_next = nodeAddr->d_next->d_next;
	}
	// then with the previous node in the list.
	if( ((caddr_t)currentNode)+currentNode->d_size ==
								(caddr_t)currentNode->d_next )
	{
		currentNode->d_size += currentNode->d_next->d_size;
		currentNode->d_next = currentNode->d_next->d_next;
	}
}

//****************************************************************************
//
//	print() - print some info about the current block map
//
//****************************************************************************
void					Mem_printVarSizeList()
{
	size_t				usedSize = 0;
	size_t				freeSize = 0;
	size_t				usedCount = 0;
	size_t				freeCount = 0;
	
	fprintf( stderr, "\n" );
	fprintf( stderr, "Used list:\n" );
	node_ptr			node = s_usedList;
	while( node->d_next != &s_usedListHead )
	{
		++usedCount;
		usedSize += node->d_size;
		fprintf( stderr, "0x%8.8x:%d bytes\n", node, node->d_size );
		node = node->d_next;
	}
	fprintf( stderr, "\n" );
	fprintf( stderr, "Free list:\n" );
	node = s_freeList;
	while( node->d_next != &s_freeListHead )
	{
		++freeCount;
		freeSize += node->d_size;
		fprintf( stderr, "0x%8.8x:%d bytes\n", node, node->d_size );
		node = node->d_next;
	}

	fprintf( stderr, "\n" );
	fprintf( stderr, "Counts:\n" );
	fprintf( stderr, "Used Count:\t%d\n", usedCount );
	fprintf( stderr, "Used Size:\t%d\n", usedSize );
	fprintf( stderr, "Free Count:\t%d\n", freeCount );
	fprintf( stderr, "Free Size:\t%d\n", freeSize );
	fprintf( stderr, "Total Size:\t%d\n", usedSize + freeSize );
}


