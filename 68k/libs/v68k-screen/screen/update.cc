/*
	update.cc
	---------
*/

#include "screen/update.hh"

// POSIX
#include <fcntl.h>
#include <unistd.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// raster
#include "raster/relay.hh"
#include "raster/relay_detail.hh"

// v68k-screen
#include "screen/storage.hh"


#ifdef __MWERKS__
#pragma exceptions off
#endif


namespace v68k   {
namespace screen {

using raster::sync_relay;


sync_relay* the_sync_relay;


struct end_sync
{
	~end_sync()
	{
		if ( the_sync_relay )
		{
			terminate( *the_sync_relay );
		}
	}
};

#ifndef __RELIX__

static end_sync finally_end_sync;

static const char* update_fifo = getenv( "GRAPHICS_UPDATE_SIGNAL_FIFO" );

#else

const char* update_fifo;

#endif


void update()
{
#ifdef __RELIX__
	
	return;
	
#endif
	
	static void* previous_frame;
	
	if ( previous_frame )
	{
		if ( memcmp( previous_frame, the_screen_buffer, the_screen_size ) == 0 )
		{
			return;
		}
	}
	
	if ( the_sync_relay != 0 )  // NULL
	{
		if ( update_fifo )
		{
			++the_sync_relay->seed;
			
			close( open( update_fifo, O_RDONLY | O_NONBLOCK ) );
		}
		else
		{
			raster::broadcast( *the_sync_relay );
		}
	}
	
	if ( previous_frame == NULL )
	{
		previous_frame = malloc( the_screen_size );
	}
	
	if ( previous_frame )
	{
		memcpy( previous_frame, the_screen_buffer, the_screen_size );
	}
}

}  // namespace screen
}  // namespace v68k
