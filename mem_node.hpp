#ifndef __MEM_NODE_HPP__
#define __MEM_NODE_HPP__

//	get size_t and caddr_t
#include <sys/types.h>

struct MemBitmap;

// This struct assumes 32-bit pointers
struct MemNode
{
	// Size of the blocks this node is managing.
	// Used for offset calculations.
	long		d_size;

	// Pointer to the bitmap that tracks used and unused blocks
	// for this cluster
	MemBitmap*	d_bitMap;

	// The cluster of pages this node manages
	caddr_t		d_cluster;

	// Pointers to make a linked list
	MemNode*	d_previousNode;
	MemNode*	d_nextNode;

	// How many blocks of this node are in use
	long		d_count;

	// padding to take this to 32 bytes, the next power of 2
	// so that some multiple of these will fit into a cluster
	caddr_t		d_padding2;
	caddr_t		d_padding3;
};

// Create a new node
MemNode*		MemNode_create( size_t a_size );

// Destroy a node
void			MemNode_destroy( MemNode* a_nodeToDestroy );

// Look for a block inside a cluster managed by this node
caddr_t			MemNode_findBlock( MemNode* a_whereToLook );

// Look for a block inside a cluster managed by this node
void			MemNode_releaseBlock( MemNode* a_whereToLook,
									  caddr_t	a_blockToRelease );

#endif // __MEM_NODE_HPP__
