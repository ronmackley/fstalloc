#include <stdlib.h>
#include <stdio.h>

#include "mem_vsiz.hpp"

int main()
{

	for( int i2=0; i2< 1000; i2++ )
	{
		void*		ptr[1000];
		size_t			index=0;
	
		for(index = 0; index <= 1000; index++ )
			ptr[index] = Mem_varSizeAlloc( rand() % 1000 );
 		Mem_printVarSizeList();
 		for(index=0 ; index < 1000; index++ )
		for(index=999 ; index >= 0; index-- )
			Mem_varSizeFree( (caddr_t)ptr[index] );
 		Mem_printVarSizeList();
		for(index = 0; index < 1000; index++ )
			ptr[index] = malloc( rand() % 1000 );
		for(index=999 ; index >= 0; index-- )
			free( (caddr_t)ptr[index] );
	}
	return 0;
}
