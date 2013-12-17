/*
	config/sockets.cc
	-----------------
*/

// POSIX
#include <sys/socket.h>

// relix-kernel
#include "relix/config/mini.hh"
#include "relix/syscall/accept.hh"
#include "relix/syscall/bind.hh"
#include "relix/syscall/connect.hh"
#include "relix/syscall/listen.hh"

// Genie
#include "Genie/SystemCallRegistry.hh"


#ifndef CONFIG_SOCKETS
#define CONFIG_SOCKETS  (!CONFIG_MINI)
#endif


namespace {

using relix::accept;
using relix::bind;
using relix::connect;
using relix::listen;


#if CONFIG_SOCKETS

#pragma force_active on

REGISTER_SYSTEM_CALL( socketpair  );
REGISTER_SYSTEM_CALL( socket      );
REGISTER_SYSTEM_CALL( bind        );
REGISTER_SYSTEM_CALL( listen      );
REGISTER_SYSTEM_CALL( accept      );
REGISTER_SYSTEM_CALL( connect     );
REGISTER_SYSTEM_CALL( getsockname );
REGISTER_SYSTEM_CALL( getpeername );
REGISTER_SYSTEM_CALL( shutdown    );

#pragma force_active reset

#endif

}  // namespace

