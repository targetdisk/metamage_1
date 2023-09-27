/*
	splode.cc
	---------
*/

#include "splode.hh"

// SDL
#include <SDL2/SDL.h>

// POSIX
#include <SDL2/SDL_events.h>
#include <unistd.h>

// splode
#include "splode/splode.hh"

#define WRITE( buffer )  write( STDOUT_FILENO, &buffer, sizeof buffer )

bool swap_Command_and_Option;

void send_key_event( SDL_KeyboardEvent e )
{
}

void send_button_event( SDL_MouseButtonEvent e )
{
}

void send_motion_event( SDL_MouseMotionEvent e )
{
}
