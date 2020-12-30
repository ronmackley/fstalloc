#ifndef			__MEM_ALOC_HPP__
#include		"mem_aloc.hpp"
#endif			// __MEM_ALOC_HPP__

#ifndef			__MEM_NODE_HPP__
#include		"mem_node.hpp"
#endif			// __MEM_NODE_HPP__

#ifndef			__MEM_CLST_HPP__
#include		"mem_clst.hpp"
#endif			// __MEM_CLST_HPP__

#ifndef			__MEM_VSIZ_HPP__
#include		"mem_vsiz.hpp"
#endif			// __MEM_VSIZ_HPP__

#include		<stdio.h>
#include		<unistd.h>

namespace
{
	// local function to set up the allocationBlock and nodeClusterMap
	bool		initMasterTables();

	// the master tables that manage allocations
	MemNode**	s_masterAllocationTable = NULL;

	// performance tracking counter
	long		s_allocationRequests = 0;

	// Constants
	const long	OVERFLOW_POOL = -1;
	const long	LARGEST_MANAGED_INDEX = 9;
	const long	LARGEST_MANAGED_ALLOCATION = 32 << LARGEST_MANAGED_INDEX;

	//************************************************************************
	//
	//	::initMasterTables() - File local function to initialize the memory
	//						 allocator.
	//
	//	NOTE:
	//		This function builds the following tables:
	//			 s_masterAllocationTable
	//			 s_nodeClusterMap
	//
	//************************************************************************
	bool			initMasterTables()
	{
		// allocate a cluster to hold the master allocation table
		// This is a bit of overkill, but it will due for now
		s_masterAllocationTable = (MemNode**)Cluster_request();
		if( s_masterAllocationTable == NULL )
		{
			return false;
		}

		for( long index = 0; index <= LARGEST_MANAGED_INDEX; index++ )
		{
			// Create a root node for each fixed size allocation category	   
			s_masterAllocationTable[index] = MemNode_create( index+5 );
			if( s_masterAllocationTable[index] == NULL )
			{
				return false;
			}
		}
		return true;
	}
}


//***************************************************************************
//
//	Mem_allocateHunk() - allocate a hunk of memory
//
//	ARGUMENTS:
//		a_howBig - the requested size
//
//	RETURNS:
//		pointer to allocated hunk
//
//	NOTE:
//
//		This function manages two tables:
//			 s_masterAllocationTable
//			 s_nodeMapTable
//
//		s_masterAllocationTable is a table of MemNodes. It is used
//		to manage allocations of various sizes. Each node in this table
//		manages allocations of successively larger powers of two bytes.
//		(32, 64, 128, ... )
//
//		s_nodeMapTable provides a reverse map of allocation address
//		to MemNode address. This way, when a hunk is released, the
//		MemNode can be notified.
//
//		These tables are created by the initMasterTables() function
//
//***************************************************************************
caddr_t			Mem_allocateHunk( size_t a_howBig )
{
	// Inceremnt the overall allocation request count
	s_allocationRequests++;
	
	if( s_masterAllocationTable == NULL )
	{
		if( ::initMasterTables() == false )
		{
			fprintf( stderr, "Mem: cannot initMasterTables()\n" );
			_exit(-1);
		}
	}

	// Hold which size category should this allocation go into
	long		masterAllocationIndex = 0;

	// Add bytes to hold a pointer to the MemNode for this
	a_howBig += sizeof(caddr_t);
	
	long		poolIndex = a_howBig;

	// if this is too big to be handled by the master
	// allocation table, put it into the overflow bin
	if( poolIndex > LARGEST_MANAGED_ALLOCATION )
	{
		masterAllocationIndex = OVERFLOW_POOL;
	}
	// Otherwise, we have a special category for this
	// size of allocation
	else
	{
		// shift of anything less than 32
		poolIndex >>= 5;
		while( masterAllocationIndex < LARGEST_MANAGED_INDEX )
		{
			// if there are no more bits left in poolIndex,
			// masterAllocationIndex contains the index into the
			// masterAllocationTable of the node that manages this
			// size allocation
			if( poolIndex == 0 )
			{
				break;
			}
			// if we have not shifted off the last bit,
			// increment the index, shift, and continue.
			masterAllocationIndex++;
			poolIndex >>= 1;
		}
	}
	
	// Now that we've determined the bin for this allocation
	// request the memory and return it to the caller

	if( masterAllocationIndex == OVERFLOW_POOL )
	{
		// This request is too big to be handled in our fixed size
		// pools. Put in into the overflow pool.
		caddr_t	  returnAddr = Mem_varSizeAlloc( a_howBig );
		// Set the managing node to be NULL to mark this
		// as being managed by the variable size allocator.
		*(void**)returnAddr = (void*)NULL;
		return returnAddr+sizeof(MemNode*);
	}

	// Otherwise, get the MemNode at masterAllocationIndex
	MemNode*   memNodePtr =
					(MemNode*)s_masterAllocationTable[masterAllocationIndex];

	// and use that memNode to get a hunk for the request
	while( 1 )
	{
		caddr_t	newBlock = MemNode_findBlock( memNodePtr );

		// Check whether this MemNode could fulfill the request
		if( newBlock == NULL )
		{
			// If not, we need to go to the next MemNode that handles
			// requests of this size.
			// Save the previous node so we can link to it
			// if necessary.
			MemNode*	prevNode = memNodePtr;
			memNodePtr = memNodePtr->d_nextNode;
			// If the node is not yet initialized, make it so
			if( memNodePtr == NULL )
			{
				// pass in the shift factor to get the size of the block
				memNodePtr = MemNode_create( masterAllocationIndex+5 );
				// Return an error if a MemNode could not be allocated
				if( memNodePtr == NULL )
				{
					return NULL;
				}
				//Now that we have a new valid node, link it in the front
				memNodePtr->d_nextNode =
								s_masterAllocationTable[masterAllocationIndex];
				s_masterAllocationTable[masterAllocationIndex] = memNodePtr;
			}
			continue;
		}
		// Found a block
		else
		{
			// Add a pointer to the managing node, so we can
			// delete the block quickly
			*(caddr_t*)newBlock = (caddr_t)memNodePtr;
			return newBlock+sizeof(MemNode*);
		}
	}
}


//***************************************************************************
//
//	Mem_releaseHunk() - mark a hunk as unused
//
//	ARGUMENTS:
//	
//		a_hunkToRelease - address of the hunk to release
//
//	This function first finds the node that manages the cluster that
//	contains a_hunkToRelease. Then it marks it as unused.
//
//***************************************************************************
void			Mem_releaseHunk( caddr_t		a_hunkToRelease )
{

	// The word before the hunk starts points to the MemNode that
	// manages the address
	
	MemNode*		managingNode =
							*(MemNode**)(a_hunkToRelease-sizeof(MemNode*));


	// If the managing node is NULL, then the variable size allocator
	// manages it.
	if( managingNode == NULL )
	{
		// Note that it varSizeFree will do nothing if it
		// cannot find this address
		Mem_varSizeFree( a_hunkToRelease-sizeof(void*) );
	}
	else
	{
		// Otherwise, release the hunk from it's node
		MemNode_releaseBlock( managingNode, a_hunkToRelease );
	}
}



//***************************************************************************
//
//	Mem_printCounts() - dump the counts of each size allocation
//
//***************************************************************************
void			Mem_printCounts()
{
	fprintf( stderr, "Total requests: %d\n", s_allocationRequests );
	long	size = 32;
	for( long index = 0; index <= LARGEST_MANAGED_INDEX; index++ )
	{
		fprintf( stderr, "%d bytes:\t%d\n", size,
				((*s_masterAllocationTable)[index]).d_count );
		size <<= 1;
	}

}
