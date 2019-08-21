/*
	accept.hh
	---------
*/

#ifndef RELIX_SYSCALL_ACCEPT_HH
#define RELIX_SYSCALL_ACCEPT_HH

// POSIX
#ifdef __APPLE__
// This is needed to compile with Mac OS X 10.3's headers.
#include <sys/types.h>
#endif
#include <sys/socket.h>


namespace relix
{
	
	int accept( int listener, struct sockaddr* addr, socklen_t* addrlen );
	
}

#endif
