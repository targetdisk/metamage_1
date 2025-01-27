/*
	raster_task.cc
	--------------
*/

#include "amicus/raster_task.hh"

// Mac OS X
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

// POSIX
#include <fcntl.h>
#include <unistd.h>

// rasterlib
#include "raster/raster.hh"
#include "raster/relay_detail.hh"
#include "raster/sync.hh"

// poseven
#include "poseven/types/thread.hh"


namespace amicus
{

static bool monitoring;

static poseven::thread raster_thread;

static
raster::sync_relay* find_sync( const raster::raster_load& load )
{
	using namespace raster;
	
	raster_note* sync = find_note( *load.meta, Note_sync );
	
	return (sync_relay*) data( sync );
}

static
void raster_event_loop( raster::sync_relay* sync )
{
	const OSType   eventClass = kEventClassAmicus;
	const uint32_t eventID    = kEventAmicusUpdate;
	
	EventQueueRef queue = GetMainEventQueue();
	
	EventRef update;
	CreateEvent( NULL, eventClass, eventID, 0, kEventAttributeNone, &update );
	
	uint32_t seed = 0;
	
	while ( monitoring  &&  sync->status == raster::Sync_ready )
	{
		while ( monitoring  &&  seed == sync->seed )
		{
			close( open( UPDATE_FIFO, O_WRONLY ) );
		}
		
		seed = sync->seed;
		
		PostEventToQueue( queue, update, kEventPriorityStandard );
	}
	
	ReleaseEvent( update );
}

static
void* raster_thread_entry( void* arg )
{
	OSStatus err;
	
	raster::sync_relay* sync = (raster::sync_relay*) arg;
	
	raster_event_loop( sync );
	
	EventRef quitEvent;
	err = CreateEvent( NULL,
	                   kEventClassApplication,
	                   kEventAppQuit,
	                   0,
	                   kEventAttributeNone,
	                   &quitEvent );
	
	EventQueueRef queue = GetMainEventQueue();
	
	err = PostEventToQueue( queue, quitEvent, kEventPriorityHigh );
	
	return NULL;
}

raster_monitor::raster_monitor( const raster::raster_load& load )
{
	GetMainEventQueue();  // initialization is thread-unsafe before 10.4
	
	raster::sync_relay* sync = find_sync( load );
	
	sync->status = raster::Sync_ready;
	sync->seed   = 0;
	
	monitoring = true;
	
	raster_thread.create( &raster_thread_entry, sync );
}

raster_monitor::~raster_monitor()
{
	monitoring = false;
	
	raster_thread.join();
}

}
