
#ifndef __MEM_CLST_HPP__
#define __MEM_CLST_HPP__

//	get size_t and caddr_t
#include <sys/types.h>

// A cluster is a fixed number of pages.

// These two routines request and release clusters.

// request a new cluster
caddr_t					Cluster_request();
caddr_t					Cluster_bigRequest( size_t* a_howBig );

// release a cluster
void					Cluster_release( caddr_t a_clusterAddress );

#endif // __MEM_CLST_HPP__
