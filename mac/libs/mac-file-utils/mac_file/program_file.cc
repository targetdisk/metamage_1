/*
	program_file.cc
	---------------
*/

#include "mac_file/program_file.hh"

// Mac OS X
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#endif

// Mac OS
#ifndef __LOWMEM__
#include <LowMem.h>
#endif

// mac-file-utils
#include "mac_file/refnum_file.hh"


namespace mac  {
namespace file {
	
#if ! __LP64__
	
	types::FSSpec program_file()
	{
		return refnum_file( LMGetCurApRefNum() );
	}
	
#else
	
	void dummy()
	{
	}
	
#endif  // #if ! __LP64__
	
}
}
