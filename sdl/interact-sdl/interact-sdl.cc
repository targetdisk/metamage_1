/*
	interact-sdl.cc
	---------------
*/

// SDL
#include <SDL2/SDL.h>

// POSIX
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_sensor.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <fcntl.h>
#include <unistd.h>

// Standard C
#include <stdlib.h>

// more-posix
#include "more/perror.hh"

// transcodex
#include "transcode/8x_1bpp_to_8bpp.hh"

// command
#include "command/get_option.hh"

// raster
#include "raster/load.hh"
#include "raster/relay_detail.hh"
#include "raster/sync.hh"

// poseven
#include "poseven/types/thread.hh"

// interact-sdl
#include "splode.hh"

#define PROGRAM "interact-sdl"

#define STR_LEN( s )  "" s, (sizeof s - 1)

#define USAGE  "usage: " PROGRAM "--raster <raster-path>\n" \
"       where raster-path is a raster file\n"

#define ERROR( msg )  write( STDERR_FILENO, STR_LEN( PROGRAM ": " msg "\n" ) )

namespace p7 = poseven;

enum
{
	Opt_title   = 't',
	Opt_magnify = 'x',
	
	Opt_last_byte = 255,
	
	Opt_feed,
	Opt_raster,
};

static command::option options[] =
{
	{ "feed",    Opt_feed,    command::Param_required },
	{ "magnify", Opt_magnify, command::Param_required },
	{ "raster",  Opt_raster,  command::Param_required },
	{ "title",   Opt_title,   command::Param_required },
	{ NULL }
};

static const char* raster_path;

static raster::raster_load loaded_raster;

static int exit_pipe_fds[ 2 ];

static
void report_error( const char* path, uint32_t err )
{
	more::perror( PROGRAM, path, err );
}

static
raster::sync_relay* open_raster( const char* path )
{
	int raster_fd = open( path, O_RDWR );
	
	if ( raster_fd < 0 )
	{
		report_error( path, errno );
		
		exit( 1 );
	}
	
	using namespace raster;
	
	loaded_raster = play_raster( raster_fd );
	
	if ( loaded_raster.addr == NULL )
	{
		report_error( path, errno );
		
		exit( 1 );
	}
	
	close( raster_fd );
	
	raster_note* sync = find_note( *loaded_raster.meta, Note_sync );
	
	if ( ! is_valid_sync( sync ) )
	{
		report_error( path, ENOSYS );
		
		exit( 3 );
	}
	
	return (sync_relay*) data( sync );
}

static p7::thread raster_update_thread;
static p7::mutex  sdl_mutex;

static SDL_Window* window;
static SDL_Surface* window_surface;
static SDL_Surface* mac_display_surface;

static
void update_image( void )
{
	SDL_BlitSurface( mac_display_surface, NULL, window_surface, NULL );
	SDL_UpdateWindowSurface( window );
}

static
void update_loop( raster::sync_relay*  sync,
                  unsigned             width,
                  unsigned             height,
                  unsigned             weight,
                  unsigned             stride )
{
	const char* update_fifo = getenv( "GRAPHICS_UPDATE_SIGNAL_FIFO" );
	
	const raster::raster_desc& desc = loaded_raster.meta->desc;
	
	const size_t image_size = height * stride;
	
	uint32_t seed = 0;
	
	while ( sync->status == raster::Sync_ready )
	{
		while ( seed == sync->seed )
		{
			if ( update_fifo )
			{
				close( open( update_fifo, O_WRONLY ) );
			}
			else
			{
				usleep( 10000 );  // 10ms
			}
			
			p7::thread::testcancel();
		}
		
		seed = sync->seed;
		
		//p7::lock lock( sdl_mutex );
		
		SDL_LockSurface( mac_display_surface );
		
		transcode_8x_1bpp_to_8bpp( loaded_raster.addr + desc.frame * image_size, mac_display_surface->pixels, image_size );
		
		SDL_UnlockSurface( mac_display_surface );
		
		update_image();
	}
}

static
char* const* get_options( char** argv )
{
	int opt;
	
	++argv;  // skip arg 0
	
	while ( (opt = command::get_option( (char* const**) &argv, options )) > 0 )
	{
		using command::global_result;
		
		switch ( opt )
		{
			case Opt_feed:
				break;
			
			case Opt_raster:
				raster_path = global_result.param;
				break;
			
			case Opt_title:
				break;
			
			case Opt_magnify:
				break;
			
			default:
				break;
		}
	}
	
	return argv;
}

static
void* raster_update_start( void* arg )
{
	raster::sync_relay* sync = (raster::sync_relay*) arg;
	
	const raster::raster_desc& desc = loaded_raster.meta->desc;
	
	update_loop( sync, desc.width, desc.height, desc.weight, desc.stride );
	
	close( exit_pipe_fds[ 1 ] );
	
	return NULL;
}

int main( int argc, char** argv )
{
	if ( argc == 0 )
	{
		return 0;
	}
	
	char* const* args = get_options( argv );
	
	int argn = argc - (args - argv);
	
	if ( raster_path == NULL )
	{
		write( STDERR_FILENO, STR_LEN( USAGE ) );
		return 2;
	}
	
	if ( const char* var = getenv( "INTERACT_SWAP_CMD_OPT" ) )
	{
		swap_Command_and_Option = atoi( var );
	}
	
	raster::sync_relay* sync = open_raster( raster_path );
	
	const raster::raster_desc& desc = loaded_raster.meta->desc;
	
	unsigned width  = desc.width;
	unsigned height = desc.height;
	unsigned depth  = desc.weight;
	unsigned stride = desc.stride;
	
	if ( depth != 1 )
	{
		ERROR( "only 1-bit-deep rasters supported" );
		return 1;
	}
	
	pipe( exit_pipe_fds );
	
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
	{
		ERROR( "SDL Init failed!" );
		return 1;
	}
	
	window = SDL_CreateWindow( "Advanced Macintosh Substitute",
	                                       SDL_WINDOWPOS_UNDEFINED,
	                                       SDL_WINDOWPOS_UNDEFINED,
	                                       width,
	                                       height,
	                                       SDL_WINDOW_SHOWN );
	
	/*
		Create two surfaces (for now).
	*/
	window_surface = SDL_GetWindowSurface( window );
	mac_display_surface = SDL_CreateRGBSurface( 0, width, height, 8, 0, 0, 0, 0 );
	if ( !mac_display_surface || !window_surface )
	{
		ERROR( "SDL surface fail!" );
		return 1;
	}
	
	raster_update_thread.create( &raster_update_start, sync );
	
	/*
		Send a NUL byte.  This may be intercepted by exhibit to indicate
		readiness, or passed on to the raster author as a null SPIEL message.
	*/
	
	write( STDOUT_FILENO, "", 1 );
	
	for ( ;; )
	{
		if ( raster_update_thread.terminated() )
		{
			break;
		}
		
		//p7::lock lock( sdl_mutex );  // NEEDED???
		
		SDL_Event event;
		
		while ( SDL_PollEvent( &event ) != 0 )
		{
			switch ( event.type )
			{
				case SDL_QUIT:
					goto cleanup;
					break;
				case SDL_WINDOWEVENT:
					update_image();
					break;
				case SDL_KEYUP:
				case SDL_KEYDOWN:
					send_key_event( event.key );
					break;
				case SDL_MOUSEMOTION:
					send_motion_event( event.motion );
					break;
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEBUTTONDOWN:
					send_button_event( event.button );
				default:
					break;
			}
		}
	}
	
cleanup:
	raster_update_thread.cancel( NULL );
	raster_update_thread.join();
	
	SDL_FreeSurface( mac_display_surface );
	mac_display_surface = nullptr;
	
	SDL_DestroyWindow( window );
	window = nullptr;
	
	SDL_Quit();
}
