/*
	getpeername.hh
	--------------
*/

#ifndef RELIX_SYSCALL_GETPEERNAME_HH
#define RELIX_SYSCALL_GETPEERNAME_HH

// POSIX
#ifdef __APPLE__
// This is needed to compile with Mac OS X 10.3's headers.
#include <sys/types.h>
#endif
#include <sys/socket.h>


namespace relix
{
	
	int getpeername( int fd, struct sockaddr* name, socklen_t* namelen );
	
}

#endif
