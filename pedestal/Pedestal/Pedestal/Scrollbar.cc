/*	============
 *	Scrollbar.cc
 *	============
 */

#include "Pedestal/Scrollbar.hh"

// Standard C++
#include <algorithm>

// Nucleus
#include "Nucleus/NAssert.h"

// Nitrogen
#include "Nitrogen/ControlDefinitions.h"
#include "Nitrogen/Gestalt.h"
#include "Nitrogen/MacWindows.h"


namespace Pedestal
{
	
	namespace N = Nitrogen;
	
	
	static inline long Gestalt_AppearanceManager()
	{
		return N::Gestalt( N::Gestalt_Selector( gestaltAppearanceAttr ), 0 );
	}
	
#if TARGET_API_MAC_CARBON
	
	static inline bool Has_AppearanceManager()  { return true; }
	
#else
	
	static bool Has_AppearanceManager()
	{
		static bool exists = Gestalt_AppearanceManager() != 0;
		
		return exists;
	}
	
#endif
	
	static inline N::ControlProcID GetControlProcIDForScrollbar()
	{
		const bool liveScrolling = Has_AppearanceManager();
		
		return liveScrolling ? N::kControlScrollBarLiveProc
		                     : N::scrollBarProc;
	}
	
	
	void Scrollbar::Activate( bool activating )
	{
		if ( activating )
		{
			//N::ShowControl( itsControl );
			N::HiliteControl( itsControl, N::kControlNoPart );
		}
		else
		{
			//N::HideControl( itsControl );
			N::HiliteControl( itsControl, N::kControlInactivePart );
		}
	}
	
	void Scrollbar::Create( const Rect& bounds, N::RefCon refCon )
	{
		ASSERT( itsControl == NULL );
		
		const N::ControlProcID procID = GetControlProcIDForScrollbar();
		
		itsControl = N::NewControl( N::GetWindowFromPort( N::GetQDGlobalsThePort() ),
		                            bounds,
		                            "\p",
		                            true,
		                            0,
		                            0,
		                            0,
		                            procID,
		                            refCon );
	}
	
	void Scrollbar::UpdateBounds( const Rect& bounds )
	{
		using namespace Nucleus::Operators;
		
		using Nucleus::operator!=;
		
		const short kScrollbarThickness = 16;
		
		ASSERT( itsControl != NULL );
		
		if ( N::GetControlBounds( itsControl ) != bounds )
		{
			N::HideControl( itsControl );
			
			N::MoveControl( itsControl, bounds.left, bounds.top );
			N::SizeControl( itsControl, kScrollbarThickness, bounds.bottom - bounds.top );
			
			N::ShowControl( itsControl );
		}
	}
	
	void Scrollbar::Clear()
	{
		if ( itsControl )
		{
			N::DisposeControl( itsControl );
			
			itsControl = NULL;
		}
	}
	
	void Scrollbar::SetValue( short value )
	{
		if ( itsControl )
		{
			N::SetControlValue( itsControl, value );
		}
	}
	
	static inline bool Has_SetControlViewSize()
	{
		return !TARGET_CPU_68K  &&  (TARGET_API_MAC_CARBON || ::SetControlViewSize != NULL );
	}
	
	void Scrollbar::Adjust( int clientLength, int offset, int viewLength )
	{
		if ( itsControl )
		{
			short max_offset = std::max( clientLength - viewLength, offset );
			
			N::SetControlMaximum( itsControl, max_offset );
			
			SetValue( offset );
			
			if ( Has_SetControlViewSize() )
			{
				::SetControlViewSize( itsControl, viewLength );
			}
		}
	}
	
}

