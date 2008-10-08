/*	=========
 *	TEView.cc
 *	=========
 */

#include "Pedestal/TEView.hh"

// Universal Interfaces
#ifndef __FONTS__
#include <Fonts.h>
#endif

// Standard C++
#include <algorithm>

// Nucleus
#include "Nucleus/NAssert.h"
#include "Nucleus/Saved.h"

// Nitrogen
#include "Nitrogen/TextEdit.h"

// ClassicToolbox
#include "ClassicToolbox/MacWindows.h"
#include "ClassicToolbox/Scrap.h"

// Pedestal
#include "Pedestal/Application.hh"
#include "Pedestal/Clipboard.hh"
#include "Pedestal/Quasimode.hh"
#include "Pedestal/Scroller.hh"


namespace Pedestal
{
	
	namespace N = Nitrogen;
	namespace NN = Nucleus;
	
	
	inline bool operator==( const TESelection& a, const TESelection& b )
	{
		return a.start == b.start  &&  a.end == b.end;
	}
	
	inline bool operator!=( const TESelection& a, const TESelection& b )
	{
		return !( a == b );
	}
	
	static TESelection Get_TESelection( TEHandle hTE )
	{
		struct TESelection result;
		
		const TERec& te = **hTE;
		
		result.start = te.selStart;
		result.end   = te.selEnd;
		
		return result;
	}
	
	
	static bool gArrowKeyMayBeChorded = false;
	
	static std::string gLastSearchPattern;
	
	
	inline bool CharIsHorizontalArrow( char c )
	{
		return (c & 0xFE) == 0x1C;
	}
	
	
	static bool pattern_match( const char* text, const char* pattern, std::size_t length )
	{
		for ( const char* end = pattern + length;  pattern != end;  ++pattern,
		                                                            ++text )
		{
			// A lower-case pattern char can match an upper-case text char.
			
			if ( *text != *pattern  &&  std::tolower( *text ) != *pattern )
			{
				return false;
			}
		}
		
		return true;
	}
	
	static short TESearch( TEHandle            hTE,
	                       const TESelection&  selection,
	                       const std::string&  pattern,
	                       bool                backward,
	                       bool                matchAtPosition )
	{
		short increment = backward ? -1 : 1;
		
		short position = pattern.size() == 1 ? backward ? selection.start - pattern.size()
		                                                : selection.end
		               : selection.start + !matchAtPosition * increment;
		
		short teLength = hTE[0]->teLength;
		
		Handle hText = hTE[0]->hText;
		
		short hLength = GetHandleSize( hText );
		
		short maxPosition = hLength - pattern.size();
		
		short limit = backward ? -1 : maxPosition + 1;
		
		while ( position != limit )
		{
			if ( pattern_match( hText[0] + position, pattern.data(), pattern.size() ) )
			{
				return position;
			}
			
			position += increment;
		}
		
		return -1;
	}
	
	
	static bool LeftAndRightArrowsKeyed()
	{
		KeyMapByteArray desiredKeys = { 0,  //  0 -  7
		                                0,  //  8 -  f
		                                0,  // 10 - 17
		                                0,  // 18 - 1f
		                                
		                                0,  // 20 - 27
		                                0,  // 28 - 2f
		                                0,  // 30 - 37
		                                0,  // 38 - 3f
		                                
		                                0,  // 40 - 47
		                                0,  // 48 - 4f
		                                0,  // 50 - 57
		                                0,  // 58 - 5f
		                                
		                                0,  // 60 - 67
		                                0,  // 68 - 6f
		                                0,  // 70 - 77
		                                1 << (0x7b & 0x07) | 1 << (0x7c & 0x07) };
		
		N::GetKeys_Result keys = N::GetKeys();
		
		return std::memcmp( keys.keyMapByteArray, desiredKeys, sizeof desiredKeys ) == 0;
	}
	
	static bool LeftOrRightArrowsKeyed()
	{
		N::GetKeys_Result keys = N::GetKeys();
		
		const UInt8 high7x = keys.keyMapByteArray[ 0x7b >> 3 ];
		
		const UInt8 leftRightArrowBits = 1 << (0x7b & 0x07) | 1 << (0x7c & 0x07);
		
		return high7x & leftRightArrowBits;
	}
	
	
	static Rect ViewRectFromBounds( const Rect& bounds )
	{
		return N::InsetRect( bounds, 4, 4 );
	}
	
	static Rect BoundsFromViewRect( const Rect& viewRect )
	{
		return N::InsetRect( viewRect, -4, -4 );
	}
	
	Rect Bounds( TEHandle hTE )
	{
		return BoundsFromViewRect( N::GetTEViewRect( hTE ) );
	}
	
	short CountLinesForDisplay( TEHandle hTE )
	{
		return N::GetTELineCount( hTE );
	}
	
	short CountLinesForEditing( TEHandle hTE )
	{
		short teLength = N::GetTELength( hTE );
		
		// An empty buffer counts as one line
		if ( teLength == 0 )
		{
			return 1;
		}
		
		short    nLines = N::GetTELineCount ( hTE );
		::Handle hText  = N::GetTETextHandle( hTE );
		
		// Find the last character in the buffer
		char c = ( *hText )[ teLength - 1 ];
		
		// If the last char is a carriage return, add an extra line
		if ( c == '\r' )
		{
			++nLines;
		}
		
		return nLines;
	}
	
	Point ViewableRange( TEHandle hTE )
	{
		Rect viewRect = N::GetTEViewRect( hTE );
		
		short viewWidth  = viewRect.right - viewRect.left;
		short viewHeight = viewRect.bottom - viewRect.top;
		
		return N::SetPt( viewWidth  /  ::CharWidth      ( 'M' ),
		                 viewHeight / N::GetTELineHeight( hTE ) );
	}
	
	Point ScrollableRange( TEHandle hTE )
	{
		return N::SetPt( 1, CountLinesForEditing( hTE ) );
	}
	
	Point ScrollStep( TEHandle hTE )
	{
		return N::SetPt( 1,
		                 N::GetTELineHeight( hTE ) );
	}
	
	static int VScrollOffset( TEHandle hTE )
	{
		int dv =   N::GetTEViewRect( hTE ).top
		         - N::GetTEDestRect( hTE ).top;
		
		return dv;
	}
	
	Point ScrollPosition( TEHandle hTE )
	{
		int dv = VScrollOffset( hTE );
		
		return N::SetPt( 0,
		                 dv == 0 ? 0
		                         : (dv - 1) / N::GetTELineHeight( hTE ) + 1 );
	}
	
	static void Resize( TEHandle hTE, const Rect& newBounds )
	{
		int dv = VScrollOffset( hTE );
		
		Rect viewRect = ViewRectFromBounds( newBounds );
		
		N::SetTEViewRect( hTE, viewRect );
		N::SetTEDestRect( hTE, N::OffsetRect( viewRect, 0, -dv ) );
		N::TECalText( hTE );
		N::InvalRect( newBounds );
	}
	
	void Resize( TEHandle hTE, short width, short height )
	{
		Rect bounds = Bounds( hTE );
		
		bounds.right = bounds.left + width;
		bounds.bottom = bounds.top + height;
		
		Resize( hTE, bounds );
	}
	
	void Scroll( TEHandle hTE, short dh, short dv )
	{
		Point scrollStep = ScrollStep( hTE );
		N::TEPinScroll( -dh * scrollStep.h, -dv * scrollStep.v, hTE );
	}
	
	static void AugmentTESelection( TEHandle hTE, const TESelection& more )
	{
		const TERec& te = **hTE;
		
		N::TESetSelect( std::min( te.selStart, more.start ),
		                std::max( te.selEnd,   more.end   ), hTE );
	}
	
	
	static void CustomClickLoop()
	{
		ClickableScroller::ClickLoop();
	}
	
	// This gets set the first time we call TENew().
	static ::TEClickLoopUPP gSystemClickLoop = NULL;
	
#if TARGET_CPU_68K && !TARGET_RT_MAC_CFM
	
	static pascal asm void MasterTEClickLoop()
	{
		MOVEA.L		gSystemClickLoop,A0	;  // load the default clickLoop
		
		JSR			(A0)				;  // invoke it
		
		MOVEM.L		D1-D2/A1,-(SP)		;  // save registers
		JSR			CustomClickLoop		;  // invoke our custom clickLoop
		MOVEM.L		(SP)+,D1-D2/A1		;  // restore registers
		
		MOVEQ		#1,D0				;  // return true by clearing the zero flag
		RTS
	}
	
#else
	
	static pascal Boolean MasterTEClickLoop( TEPtr pTE )
	{
		(void) ::InvokeTEClickLoopUPP( pTE, gSystemClickLoop );
		
		CustomClickLoop();
		
		return true;
	}
	
#endif
	
	static ::TEClickLoopUPP gMasterClickLoop = ::NewTEClickLoopUPP( MasterTEClickLoop );
	
	
	static int SetTextAttributes()
	{
		::TextFont( kFontIDMonaco );
		::TextSize( 9 );
		
		return 0;
	}
	
	
	TEView::TEView( const Rect&  bounds,
	                Initializer  /**/ ) : itsTE( ( SetTextAttributes(),
	                                               N::TENew( ViewRectFromBounds( bounds ) ) ) ),
	                                      itsSelectionPriorToSearch(),
	                                      itsSelectionPriorToArrow ()
	{
		N::TEAutoView( true, itsTE );  // enable auto-scrolling
		
		static ::TEClickLoopUPP clickLoop = gSystemClickLoop = itsTE.Get()[0]->clickLoop;
		
		itsTE.Get()[0]->clickLoop = gMasterClickLoop;
	}
	
	void TEView::Idle( const EventRecord& )
	{
		N::TEIdle( itsTE );
		
		AdjustSleepForTimer( ::GetCaretTime() );
		
		if ( !LeftOrRightArrowsKeyed() )
		{
			gArrowKeyMayBeChorded = false;
		}
	}
	
	void TEView::MouseDown( const EventRecord& event )
	{
		N::TEClick( N::GlobalToLocal( event.where ),
		            event.modifiers & shiftKey,
		            itsTE );
	}
	
	inline bool NewContentReplacesSelection()
	{
		// True in Mac HIG, false in The Humane Interface
		return false;
	}
	
	inline bool KeyIsAllowedAgainstSelection( char c, TEHandle aTE )
	{
		if ( NewContentReplacesSelection() )
		{
			return true;
		}
		
		// Allow control keys always (backspace, arrows)
		// Allow content keys only when selection is empty (insertion point)
		return c < 0x20  ||  aTE[0]->selStart == aTE[0]->selEnd;
	}
	
	static EventRecord gLastTextEditKeyDownEvent;
	
	static unsigned gTextEditKeyCount = 0;
	
	inline bool AutoKeyRequiresThreeStrikes()
	{
		// False in Mac HIG
		return true;
	}
	
	bool TEView::KeyDown( const EventRecord& event )
	{
		if ( AutoKeyRequiresThreeStrikes()  &&  event.what == autoKey  &&  gTextEditKeyCount < 3 )
		{
			// Suppress auto-key until after three consecutive key-downs
			return true;
		}
		
		if ( event.what == keyDown )
		{
			if (    event.message   == gLastTextEditKeyDownEvent.message
			     && event.modifiers == gLastTextEditKeyDownEvent.modifiers )
			{
				++gTextEditKeyCount;
			}
			else
			{
				gTextEditKeyCount = 1;
			}
		}
		
		gLastTextEditKeyDownEvent = event;
		
		char c = event.message & charCodeMask;
		
		const UInt16 bothShiftKeys = shiftKey | rightShiftKey;
		
		UInt16 shifted = event.modifiers & bothShiftKeys;
		
		if ( c == ' '  &&  shifted  &&  (event.modifiers & cmdKey) )
		{
			if ( shifted == bothShiftKeys )
			{
				N::SysBeep();
				
				return true;
			}
			
			bool backward = shifted == shiftKey;
			
			TESelection selection = Get_TESelection( Get() );
			
			short match = TESearch( Get(), selection, gLastSearchPattern, backward, false );
			
			if ( match == -1 )
			{
				N::SysBeep();
			}
			else
			{
				SetSelection( match, match + gLastSearchPattern.size() );
			}
		}
		else if ( event.modifiers & controlKey  &&  CharIsHorizontalArrow( c ) )
		{
			KeyMapByteArray desiredKeys = { 0,  //  0 -  7
			                                0,  //  8 -  f
			                                0,  // 10 - 17
			                                0,  // 18 - 1f
			                                
			                                0,  // 20 - 27
			                                0,  // 28 - 2f
			                                0,  // 30 - 37
			                                1 << (0x3b & 0x07),
			                                
			                                0,  // 40 - 47
			                                0,  // 48 - 4f
			                                0,  // 50 - 57
			                                0,  // 58 - 5f
			                                
			                                0,  // 60 - 67
			                                0,  // 68 - 6f
			                                0,  // 70 - 77
			                                1 << (0x7b & 0x07) | 1 << (0x7c & 0x07) };
			
			N::GetKeys_Result keys = N::GetKeys();
			
			if ( std::memcmp( keys.keyMapByteArray, desiredKeys, sizeof desiredKeys ) == 0 )
			{
				AugmentTESelection( itsTE, itsSelectionPriorToSearch );
			}
		}
		else if ( CharIsHorizontalArrow( c ) )
		{
			const bool keyed = LeftAndRightArrowsKeyed() && gArrowKeyMayBeChorded;
			
			if ( !keyed )
			{
				itsSelectionPriorToArrow = Get_TESelection( Get() );
				
				N::TEKey( c, itsTE );
				
				gArrowKeyMayBeChorded = true;
			}
			else
			{
				// Restore selection undone by previous arrow key
				SetSelection( itsSelectionPriorToArrow );
				
				AugmentTESelection( itsTE, itsSelectionPriorToSearch );
			}
		}
		else if ( KeyIsAllowedAgainstSelection( c, itsTE ) )
		{
			N::TEKey( c, itsTE );
		}
		else
		{
			static UInt32 lastBeep = 0;
			
			if ( event.when - lastBeep > 45 )
			{
				lastBeep = event.when;
				
				N::SysBeep();
			}
		}
		
		return true;
	}
	
	static void DrawQuasimodeFrame( Rect frame )
	{
		N::InsetRect( frame, 1, 1 );
		
		N::FrameRect( frame );
	}
	
	class TESearchQuasimode : public Quasimode
	{
		private:
			TEView&                     itsView;
			bool                        itSearchesBackward;
			UInt16                      itsModifierMask;
			TESelection                 itsSavedSelection;
			std::vector< TESelection >  itsMatches;
			std::string                 itsPattern;
		
		public:
			TESearchQuasimode( TEView& view, bool backward );
			
			~TESearchQuasimode();
			
			bool KeyDown( const EventRecord& event );
	};
	
	
	TESearchQuasimode::TESearchQuasimode( TEView&  view,
	                                      bool     backward ) : itsView           ( view     ),
	                                                            itSearchesBackward( backward ),
	                                                            itsModifierMask   ( backward ? shiftKey : rightShiftKey ),
	                                                            itsSavedSelection ( Get_TESelection( view.Get() ) )
	{
		DrawQuasimodeFrame( itsView.Bounds() );
	}
	
	static const RGBColor gRGBBlack = {     0,     0,     0 };
	static const RGBColor gRGBWhite = { 65535, 65535, 65535 };
	
	TESearchQuasimode::~TESearchQuasimode()
	{
		N::RGBForeColor( gRGBWhite );
		
		DrawQuasimodeFrame( itsView.Bounds() );
		
		N::RGBForeColor( gRGBBlack );
		
		if ( Get_TESelection( itsView.Get() ) != itsSavedSelection )
		{
			gLastSearchPattern = itsPattern;
			
			itsView.itsSelectionPriorToSearch = itsSavedSelection;
		}
	}
	
	static char GetTranslatedKeyFromEvent( const EventRecord& event, UInt16 ignoredModifierMask )
	{
		static UInt32 state = 0;
		
		Handle kchr = GetResource( 'KCHR', 0 );
		
		N::ResError();
		
		ASSERT(  kchr != NULL );
		ASSERT( *kchr != NULL );
		
		UInt16 keyCode = (event.message & keyCodeMask) >> 8;
		
		keyCode |= event.modifiers & (0xff00 - ignoredModifierMask);
		
		UInt32 key = KeyTranslate( *kchr, keyCode, &state );
		
		const char c = key & 0xff;
		
		return c;
	}
	
	bool TESearchQuasimode::KeyDown( const EventRecord& event )
	{
		if ( event.what == autoKey )
		{
			return true;  // eat auto-repeat keys
		}
		
		const UInt16 ignoredModifierMask = itsModifierMask;
		
		const char c = GetTranslatedKeyFromEvent( event, ignoredModifierMask );
		
		if ( CharIsHorizontalArrow( c ) )
		{
			const bool goingToSearchBackward = c == kLeftArrowCharCode;
			
			if ( itSearchesBackward != goingToSearchBackward )
			{
				itSearchesBackward = goingToSearchBackward;
				
				if ( !itsPattern.empty() )
				{
					// FIXME:  Rerun the search in the other direction
					itsPattern.clear();
					itsMatches.clear();
					
					itsView.SetSelection( itsSavedSelection );
					
					N::SysBeep( 30 );
				}
			}
		}
		else if ( c == 0x08 )
		{
			if ( itsPattern.empty() )
			{
				N::SysBeep();
			}
			else
			{
				TESelection match = itsMatches.back();
				
				itsMatches.pop_back();
				
				itsPattern.resize( itsPattern.size() - 1 );  // pop_back() isn't standard
				
				itsView.SetSelection( match );
			}
		}
		else
		{
			itsPattern += c;
			
			short position = itsView.Get()[0]->selStart;
			
			TESelection selection = Get_TESelection( itsView.Get() );
			
			itsMatches.push_back( selection );
			
			short match = TESearch( itsView.Get(), selection, itsPattern, itSearchesBackward, true );
			
			if ( match == -1 )
			{
				itsView.SetSelection( itsSavedSelection );
				
				N::SysBeep();
			}
			else
			{
				itsView.SetSelection( match, match + itsPattern.size() );
			}
		}
		
		return true;
	}
	
	boost::shared_ptr< Quasimode > TEView::EnterShiftSpaceQuasimode( const EventRecord& event )
	{
		bool backward = event.modifiers & shiftKey;
		
		boost::shared_ptr< Quasimode > mode( new TESearchQuasimode( *this, backward ) );
		
		return mode;
	}
	
	void TEView::Activate( bool activating )
	{
		if ( activating )
		{
			N::TEActivate( itsTE );
		}
		else
		{
			N::TEDeactivate( itsTE );
		}
		
		gArrowKeyMayBeChorded = false;
	}
	
	void TEView::Update()
	{
		Rect bounds = Pedestal::Bounds( itsTE.Get() );
		
		Rect top = bounds;
		top.bottom = top.top + 4;
		
		Rect left = bounds;
		left.right = left.left + 4;
		
		Rect bottom = bounds;
		bottom.top = bottom.bottom - 4;
		
		Rect right = bounds;
		right.left = right.right - 4;
		
		N::EraseRect( top    );
		N::EraseRect( left   );
		N::EraseRect( bottom );
		N::EraseRect( right  );
		
		N::TEUpdate( bounds, itsTE );
		
		int textHeight = CountLinesForDisplay( itsTE ) * N::GetTELineHeight( itsTE );
		
		Rect viewRect = N::GetTEViewRect( itsTE );
		short viewHeight = viewRect.bottom - viewRect.top;
		
		Rect destRect = N::GetTEDestRect( itsTE );
		destRect.bottom = destRect.top + textHeight;
		
		// If the bottom of the text doesn't reach the bottom of the viewing area,
		if ( destRect.bottom < viewRect.bottom )
		{
			// then below the text is a blank space, which TEUpdate() ignores.
			// So we need to erase it ourselves.
			viewRect.top = destRect.bottom;
			N::EraseRect( viewRect );
		}
	}
	
	bool TEView::UserCommand( MenuItemCode code )
	{
		switch ( code )
		{
			// Edit
			case 'undo':
				//Undo();
				break;
			
			case 'cut ':
				Cut();
				break;
			
			case 'copy':
				Copy();
				break;
			
			case 'past':  // kHICommandPaste
			case 'pste':
				Paste();
				break;
			
			case 'sall':  // kHICommandSelectAll
			case 'slct':
				SelectAll();
				break;
			
			default:
				return false;
				break;
		}
		
		return true;
	}
	
	bool TEView::SetCursor( Point location, RgnHandle /*mouseRgn*/ )
	{
		if ( N::PtInRect( location, Bounds() ) )
		{
			N::SetCursor( N::GetCursor( N::iBeamCursor ) );
			return true;
		}
		else
		{
			return false;
		}
	}
	
	int TEView::AppendChars( const char* data, unsigned int byteCount, bool updateNow )
	{
		if ( byteCount == 0 )  return 0;
		
		ASSERT( data != NULL );
		
		Handle hText = N::GetTETextHandle( itsTE );
		short len = N::GetTELength( itsTE );
		
		byteCount = std::min< unsigned int >( byteCount, 32000 - len );
		
		N::SetHandleSize( hText, len + byteCount );
		
		std::copy( data, data + byteCount, *hText + len );
		
		std::replace( *hText + len,
		              *hText + len + byteCount,
		              '\n',
		              '\r' );
		
		N::SetTELength( itsTE, len + byteCount );
		N::TECalText( itsTE );
		N::TESetSelect( 32767, 32767, itsTE );
		
		if ( updateNow )
		{
			N::GrafPtr port = N::GetTEPort( itsTE );
			
			// This is correct but MWPro6 breaks on it
			//NN::Saved< N::Port_Value > savedPort( port );
			
			NN::Saved< N::Port_Value > savedPort;
			
			N::SetPort( port );
			
			Update();
		}
		
		return byteCount;
	}
	
	void TEView::Cut()
	{
		Clipboard::TECut( itsTE );
	}
	
	void TEView::Copy()
	{
		Clipboard::TECopy( itsTE );
	}
	
	void TEView::Paste()
	{
		Clipboard::TEPaste( itsTE );
	}
	
	void TEView::Clear()
	{
		N::TEDelete( itsTE );
	}
	
	void TEView::SelectAll()
	{
		N::TESetSelect( 0, 32767, itsTE );
	}
	
}

