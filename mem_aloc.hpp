#ifndef			__MEM_ALOC_H__
#define			__MEM_ALOC_H__

#include <sys/types.h>

// Allocate a hunk of memory
caddr_t			Mem_allocateHunk( size_t a_howBig );

// Release a hunk of memory
void			Mem_releaseHunk( caddr_t a_hunkToRelease );

// Print some stats
void			Mem_printCounts();
#endif			// __MEM_ALOC_H__


