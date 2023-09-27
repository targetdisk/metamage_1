/*
	splode.hh
	---------
*/

#ifndef SPLODE_HH
#define SPLODE_HH

// SDL
#include <SDL2/SDL.h>


extern bool swap_Command_and_Option;

//void send_cursor_location( int x, int y );

void send_motion_event( SDL_MouseMotionEvent );
void send_button_event( SDL_MouseButtonEvent );
void send_key_event   ( SDL_KeyboardEvent );

#endif
