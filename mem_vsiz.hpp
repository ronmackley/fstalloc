#ifndef __MEM_VSIZ_H__
#define __MEM_VSIZ_H__

// get NULL and size_t
#include <sys/types.h>
#include <stdlib.h>

caddr_t					Mem_varSizeAlloc( size_t	a_size );
void					Mem_varSizeFree( caddr_t	a_addr );
void					Mem_printVarSizeList();


#endif //__MEM_VSIZ_H__
