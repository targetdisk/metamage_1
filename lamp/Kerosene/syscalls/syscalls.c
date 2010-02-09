/*
	syscalls.c
	----------
	
	Joshua Juran
*/

// Lamp
#include "lamp/syscalls.h"


static void* global_dispatcher;

extern void _set_dispatcher( void* address );

void _set_dispatcher( void* address )
{
	global_dispatcher = address;
}

extern int syscall( int number, ... );

#ifdef __MC68K__
	
	static asm void SystemCall()
	{
		MOVEA.L		global_dispatcher,A0	;  // load the dispatcher's address
											;  // syscall index already on stack
		JMP			(A0)					;  // jump to dispatcher
		
		// Not reached
	}
	
	asm int syscall( int number, ... )
	{
		MOVE.L  4(SP), D0    // copy system call number to d0
		MOVE.L  (SP)+, (SP)  // overwrite it with the return address, and pop
		
		BRA     SystemCall
	}
	
	#define DEFINE_STUB_7F( name )    \
		extern void name();           \
		asm void name()               \
		{                             \
			MOVEQ #__NR_##name,D0  ;  \
			BRA SystemCall         ;  \
		}
	
	#define DEFINE_STUB( name )       \
		extern void name();           \
		asm void name()               \
		{                             \
			MOVE.W #__NR_##name,D0 ;  \
			BRA SystemCall         ;  \
		}
	
#endif

#ifdef __POWERPC__
	
	extern void __ptr_glue();
	
	static asm void SystemCall()
	{
		nofralloc
		
		mflr	r0						// get caller's return address
		stw		r0,8(SP)				// store it in caller's stack frame
		
		stwu	SP,-64(SP)				// allocate our own stack frame
		
		lwz		r12,global_dispatcher	// load dispatcher T-vector
		
		bl		__ptr_glue				// cross-TOC call
		nop								// synchronize pipeline
		lwz		RTOC,20(SP)				// restore our RTOC
		
		addi	SP,SP,64				// deallocate our stack frame
		
		lwz		r0,8(SP)				// reload caller's return address
		mtlr	r0						// load it into the link register
		blr								// return
	}
	
	asm int syscall( int number, ... )
	{
		nofralloc
		
		// copy system call number to r11
		mr  r11,r3
		
		// bump up to six arguments into the correct registers
		mr  r3,r4
		mr  r4,r5
		mr  r5,r6
		mr  r6,r7
		mr  r7,r8
		mr  r8,r9
		
		b   SystemCall
	}
	
	#define DEFINE_STUB( name )   \
		extern void name();       \
		asm void name()           \
		{                         \
			li r11,__NR_##name ;  \
			b SystemCall       ;  \
		}
	
	#define DEFINE_STUB_7F( name )  DEFINE_STUB( name )
	
#endif

DEFINE_STUB_7F( InitProc )
DEFINE_STUB_7F( _exit )
DEFINE_STUB_7F( vfork_start )
DEFINE_STUB_7F( read  )
DEFINE_STUB_7F( write )
//DEFINE_STUB_7F( open  )
DEFINE_STUB_7F( close )
DEFINE_STUB_7F( waitpid )
DEFINE_STUB_7F( realpath_k )

//DEFINE_STUB_7F( unlink )
DEFINE_STUB_7F( execve )
DEFINE_STUB_7F( chdir  )

//DEFINE_STUB_7F( mknod )
//DEFINE_STUB_7F( chmod )

DEFINE_STUB_7F( reexec )
DEFINE_STUB_7F( getcwd_k )
DEFINE_STUB_7F( lseek )
DEFINE_STUB_7F( getpid )

DEFINE_STUB_7F( ptrace )
DEFINE_STUB_7F( alarm )

DEFINE_STUB_7F( pause )
//DEFINE_STUB_7F( utime )

//DEFINE_STUB_7F( access )

DEFINE_STUB_7F( sync )
DEFINE_STUB_7F( kill )
//DEFINE_STUB_7F( rename )
//DEFINE_STUB_7F( mkdir )
//DEFINE_STUB_7F( rmdir )
//DEFINE_STUB_7F( dup )
//DEFINE_STUB_7F( pipe )
DEFINE_STUB_7F( times )

DEFINE_STUB_7F( signal )
DEFINE_STUB_7F( flock )
DEFINE_STUB_7F( ioctl )
DEFINE_STUB_7F( fcntl )

DEFINE_STUB_7F( setpgid )

DEFINE_STUB_7F( uname )
DEFINE_STUB_7F( umask )

//DEFINE_STUB_7F( dup2 )
DEFINE_STUB_7F( getppid )

DEFINE_STUB_7F( setsid )
DEFINE_STUB_7F( sigaction )

DEFINE_STUB_7F( sigsuspend )
DEFINE_STUB_7F( sigpending )

DEFINE_STUB_7F( gettimeofday )

DEFINE_STUB_7F( select )
//DEFINE_STUB_7F( symlink )

//DEFINE_STUB_7F( readlink_k )

DEFINE_STUB_7F( mmap )
DEFINE_STUB_7F( munmap )
DEFINE_STUB_7F( truncate )
DEFINE_STUB_7F( ftruncate )
DEFINE_STUB_7F( fchmod )

DEFINE_STUB_7F( setitimer )
DEFINE_STUB_7F( getitimer )
//DEFINE_STUB_7F( stat )
//DEFINE_STUB_7F( lstat )
DEFINE_STUB_7F( fstat )

DEFINE_STUB_7F( fsync )

DEFINE_STUB_7F( sigprocmask )

DEFINE_STUB( getpgid )
DEFINE_STUB( fchdir )

DEFINE_STUB( getdents )

DEFINE_STUB( writev )
DEFINE_STUB( getsid )
DEFINE_STUB( fdatasync )

DEFINE_STUB( nanosleep )

DEFINE_STUB( poll )

DEFINE_STUB( pread )
DEFINE_STUB( pwrite )

DEFINE_STUB( openat       )
DEFINE_STUB( mkdirat      )
DEFINE_STUB( mknodat      )
DEFINE_STUB( fchownat     )
//DEFINE_STUB( futimesat    )
DEFINE_STUB( fstatat      )
DEFINE_STUB( unlinkat     )
DEFINE_STUB( renameat     )
DEFINE_STUB( linkat       )
DEFINE_STUB( symlinkat    )
DEFINE_STUB( readlinkat_k )
DEFINE_STUB( fchmodat     )
DEFINE_STUB( faccessat    )

DEFINE_STUB( copyfileat )
DEFINE_STUB( updateat   )

DEFINE_STUB( socketpair )
DEFINE_STUB( socket )
DEFINE_STUB( bind )
DEFINE_STUB( listen )
DEFINE_STUB( accept )
DEFINE_STUB( connect )
DEFINE_STUB( shutdown )
DEFINE_STUB( getsockname )
DEFINE_STUB( getpeername )
DEFINE_STUB( getsockopt )
DEFINE_STUB( setsockopt )
DEFINE_STUB( send )
DEFINE_STUB( recv )
DEFINE_STUB( sendto )
DEFINE_STUB( recvfrom )

DEFINE_STUB( gethostbyname )

DEFINE_STUB( fork_and_exit )

DEFINE_STUB( pump )

DEFINE_STUB( ttypair )

DEFINE_STUB( copyfile )

DEFINE_STUB( AESendBlocking )
DEFINE_STUB( OTInetMailExchange_k )

DEFINE_STUB( utimensat )

DEFINE_STUB( dup3 )
DEFINE_STUB( pipe2 )

