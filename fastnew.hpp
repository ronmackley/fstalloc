#ifndef __FASTNEW_HPP__
#define __FASTNEW_HPP__

#include <unistd.h>

void*			operator new( size_t a_requestSize ) throw();
void*			operator new[] ( size_t a_requestSize ) throw();

// This will never raise an exception or fail in any way.
void			operator delete( void* a_addressToRelease ) throw();
void			operator delete[]( void* a_addressToRelease ) throw();

#endif
