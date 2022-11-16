/*	==========
 *	setleds.cc
 *	==========
 */

// Mac OS X
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#endif

// Mac OS
#ifndef __MACTYPES__
#include <MacTypes.h>
#endif

// POSIX
#include <unistd.h>

// Standard C
#include <stdio.h>
#include <string.h>

// iota
#include "iota/strings.hh"

// plus
#include "plus/var_string.hh"

// Arcana
#if !TARGET_API_MAC_CARBON
#include "ADB/KeyboardLEDs.hh"
#endif


namespace Nitrogen
{
	// Define namespace Nitrogen for Carbon
}

namespace N = Nitrogen;


#if !TARGET_API_MAC_CARBON


static int gChangedBits     = 0;
static int gChangedBitsMask = 0;


static inline bool IsKeyboard( const ADBDataBlock& adbData )
{
	const SInt8 kADBKeyboardDevice = 2;
	
	return adbData.devType == kADBKeyboardDevice;
}


static void PrintLEDs( UInt8 leds )
{
	const char* status[] = { "off", "on " };
	
	bool num    = leds & 1;
	bool caps   = leds & 2;
	bool scroll = leds & 4;
	
	plus::var_string message;
	
	// Reserve enough memory to hold everything, avoiding reallocation
	message.reserve( 255 );
	
	/*
	message += "# Device at ADB address ";
	
	message += address + '0';
	message += "\n";
	*/
	
	message += "Current default flags:  NumLock on    CapsLock off   ScrollLock off\n";
	message += "Current flags:          NumLock on    CapsLock off   ScrollLock off\n";
	message += "Current leds:           NumLock ";
	
	message += status[ num    ];
	message += "   " "CapsLock ";
	message += status[ caps   ];
	message += "   " "ScrollLock ";
	message += status[ scroll ];
	message += "\n";
	
	write( STDOUT_FILENO, message.data(), message.size() );
}

static
void DoLEDs( N::GetIndADB_Result adbDevice )
{
	if ( IsKeyboard( adbDevice ) )
	{
		UInt8 leds = GetLEDs( adbDevice );
		
		if ( gChangedBitsMask == 0 )
		{
			PrintLEDs( leds );
		}
		else
		{
			leds &= ~gChangedBitsMask;
			leds |= gChangedBits;
			
			SetLEDs( adbDevice, leds );
		}
	}
}

static const char* gOptionNames[] = { "num", "caps", "scroll", NULL };

#endif


int main( int argc, char** argv )
{
#if TARGET_API_MAC_CARBON
	
	write( STDERR_FILENO, STR_LEN( "setleds: ADB unsupported in Carbon\n" ) );
	
	return 1;
	
#else
	
	int count = N::CountADBs();
	
	for ( char** argp = argv + 1;  *argp != NULL;  ++argp )
	{
		const char* arg = *argp;
		
		bool on = false;
		
		if ( arg[0] == '+' )
		{
			on = true;
		}
		else if ( arg[0] != '-' )
		{
			fprintf( stderr, "setleds: unrecognized argument '%s'\n", arg );
			
			return 2;
		}
		
		for ( const char** pp = gOptionNames;  ;  ++pp )
		{
			if ( strcmp( *pp, arg + 1 ) == 0 )
			{
				int bit = pp - gOptionNames;
				
				gChangedBits     |= on << bit;
				gChangedBitsMask |= 1  << bit;
				
				break;
			}
			
			if ( *pp == NULL )
			{
				fprintf( stderr, "setleds: unrecognized led '%s'\n", arg );
				
				return 2;
			}
		}
	}
	
	N::ADBDevice_Container adbs;
	
	typedef N::ADBDevice_Container::const_iterator Iter;
	
	for ( Iter it = adbs.begin(), end = adbs.end();  it != end;  ++it )
	{
		DoLEDs( *it );
	}
	
	return 0;
	
#endif
}
