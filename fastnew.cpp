
#ifndef __FASTNEW_HPP__
#include "fastnew.hpp"
#endif

#ifndef __MEM_ALOC_HPP__
#include "mem_aloc.hpp"
#endif

void*			operator new( size_t a_requestSize ) throw()
{
//fprintf( stderr, "new: %d\n", a_requestSize );	
	return (void*)Mem_allocateHunk( a_requestSize );
}

void*			operator new[] ( size_t a_requestSize ) throw()
{
//fprintf( stderr, "Array new: %d\n", a_requestSize );
	return (void*)Mem_allocateHunk( a_requestSize );
}


// This will never raise an exception or fail in any way.
void			operator delete( void* a_addressToRelease ) throw()
{
//fprintf( stderr, "Delete: 0x%8.8x\n", a_addressToRelease );
	if( a_addressToRelease == NULL )
		return;
	Mem_releaseHunk( (caddr_t)a_addressToRelease );
}

void			operator delete[]( void* a_addressToRelease ) throw()
{
//fprintf( stderr, "Array delete: 0x%8.8x\n", a_addressToRelease );	
	if( a_addressToRelease == NULL )
		return;
	Mem_releaseHunk( (caddr_t)a_addressToRelease );
}
