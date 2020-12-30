#ifndef			__MEM_CLST_H__
#include		"mem_clst.hpp"
#endif

#include		<unistd.h>
#include		<stdlib.h>
#include		<stdio.h>
#include		<sys/mman.h>

#include		<sys/types.h>
#include		<sys/stat.h>
#include		<fcntl.h>

// linux can do anonymous mappings directly
// solaris needs /dev/zero to do it
#if				defined(LINUX)
#define			PROT_FLAGS	  (PROT_READ|PROT_WRITE)
#define			MAPPING_FLAGS (MAP_PRIVATE|MAP_ANON)
#else
#if				defined(SUN)
#define			PROT_FLAGS	  (PROT_READ|PROT_WRITE)
#define			MAPPING_FLAGS (MAP_PRIVATE|MAP_NORESERVE)
#else
#error	No Platform for mmap args.
#endif			// SUN
#endif			// LINUX

// default to 32k clusters
#define			CLUSTERSIZE 65536

// the size of a cluster
size_t	s_clusterSize = 0;
size_t	s_pageSize = 0;

// file-local variables into the unnamed namespace
namespace
{
	// descriptor to hold /dev/zero
	int s_zeroFd = -1;

	//************************************************************************
	//
	//	fullfilRequest() - get a cluster of anonymous memory
	//
	//	ARGUMENTS:
	//		a_howBig - how much memory to request
	//	
	//
	//	RETURNS:
	//		pointer to cluster
	//		NULL on error
	//
	//************************************************************************
	caddr_t			fullfilRequest( size_t a_howBig )
	{

	   // to get anonymous mapping, map on /dev/zero
	   if( s_zeroFd == -1 )
	   {
			s_zeroFd = open( "/dev/zero", O_RDWR );
			if( s_zeroFd == -1 )
			{
				perror("open /dev/zero: ");
				return NULL;
			}
		}

		caddr_t		cluster = (caddr_t) mmap( (caddr_t) 0,
											  a_howBig,
											  PROT_FLAGS,
											  MAPPING_FLAGS,
											  s_zeroFd, 0 );
		// this should return NULL on error so the app can deal with
		// a memory shortfall on its own.
		if( cluster == (caddr_t)-1 )
		{
			perror( "mmap: " );
			return NULL;
		}
		return cluster;
	}

}

//****************************************************************************
//
//	Cluster_request() - get a hunk of anonymous memory
//
//	RETURNS:
//		pointer to cluster
//		NULL on error
//
//****************************************************************************
caddr_t			Cluster_request()
{
	// if not yet defined, get the number of pagesPerClustr
	// and set the size of a cluster
	if( s_clusterSize == 0 )
	{
		// Keep this a in variable to reduce preprocessor coupling
		s_clusterSize =	CLUSTERSIZE;
	}
	return fullfilRequest( s_clusterSize );
}

//****************************************************************************
//
//	Cluster_bigRequest() - get a big hunk of anonymous memory
//
//	ARGUMENTS:
//		a_howBig - how much memory to request, populated with the
//				   actual size returned
//
//	RETURNS:
//		pointer to cluster
//		NULL on error
//
//
//	NOTE:
//		This function will return hunk of memory in a multiple of pages
//		a_howBig is only a guideline
//
//****************************************************************************
caddr_t			Cluster_bigRequest( size_t* a_howBig )
{
	// if not yet defined, get the number of pagesPerClustr
	// and set the size of a cluster
	if( s_clusterSize == 0 )
	{
		// Keep this a in variable to reduce preprocessor coupling
		s_clusterSize =	CLUSTERSIZE;
	}

	// Round the requestSize to the next nearest page size
	if(	s_pageSize == 0 )
	{
		s_pageSize = getpagesize();
	}
	
	size_t		requestSize;

	// Make sure we get at least one page
	if( *a_howBig < s_clusterSize )
	{
		requestSize = s_clusterSize;
	}
	else
	{
		requestSize = s_clusterSize * (*a_howBig/s_clusterSize);
	}

	// double the request size, for padding
	requestSize <<= 1;
	
	caddr_t		newCluster = fullfilRequest( requestSize );
	
	if( newCluster == NULL )
	{
		// No memory was allocated
		*a_howBig = 0;
	}
	else
	{
		*a_howBig = requestSize;
	}
	
	return newCluster;

}

//****************************************************************************
//
//	Cluster_release() - return a hunk of anonymous memory
//
//	ARGS:
//		a_clusterAddress - pointer to cluster to release
//
//****************************************************************************
void			Cluster_release( caddr_t a_clusterAddress )
{
	if( munmap( a_clusterAddress, s_clusterSize ) == -1 )
	{
		perror( "munmap: " );
	}
}


#ifdef TEST
//***************************************************************************
//
//	Main() - simple routine to test the cluster management routines	
//
//***************************************************************************
int main(int, char**)
{
	char*		newCluster = NULL;
	// if Cluster_request() returns NULL, the test has failed
	newCluster = (char*) Cluster_request();
	if( newCluster == NULL )
	{
		fprintf( stderr, "%50.50s:\t[%4.4s]\n", "Allocate Cluster", "FAIL" );
		::exit(-1);
	}
	fprintf( stderr, "%50.50s:\t[%4.4s]\n", "Allocate Cluster", "PASS" );

	// Write to the requested cluster.
	for( long index = 0; index < s_clusterSize; index++ )
	{
		// Need a way to check for a failed write. Set SIGSEGV?
		printf( "%x", newCluster[index] );
		newCluster[index] = 12;
	}
	fprintf( stderr, "%50.50s:\t[%4.4s]\n", "Fill Cluster", "PASS" );
}
#endif // TEST
