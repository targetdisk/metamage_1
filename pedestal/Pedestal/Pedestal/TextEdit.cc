/*	===========
 *	TextEdit.cc
 *	===========
 */

#include "Pedestal/TextEdit.hh"

// Mac OS X
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

// Mac OS
#ifndef __FONTS__
#include <Fonts.h>
#endif
#ifndef __TEXTEDIT__
#include <TextEdit.h>
#endif

// mac-sys-utils
#include "mac_sys/beep.hh"
#include "mac_sys/trap_available.hh"

// mac-qd-utils
#include "mac_qd/get_portRect.hh"
#include "mac_qd/globals/thePort.hh"

// Debug
#include "debug/assert.hh"

// Nitrogen
#include "Nitrogen/Events.hh"
#include "Nitrogen/Quickdraw.hh"
#include "Nitrogen/TextEdit.hh"

// ClassicToolbox
#include "ClassicToolbox/MacWindows.hh"

// Pedestal
#include "Pedestal/Application.hh"
#include "Pedestal/AutoKey.hh"
#include "Pedestal/Clipboard.hh"
#include "Pedestal/CurrentFocus.hh"
#include "Pedestal/IncrementalSearch.hh"


static inline
bool has_TEAutoView()
{
	enum { _TEAutoView = 0xA813 };
	
	return ! TARGET_CPU_68K  ||  mac::sys::trap_available( _TEAutoView );
}

namespace Pedestal
{
	
	namespace N = Nitrogen;
	
	
	static bool gExtendingSelection = false;
	
	static short gSelectionAnchor, gSelectionExtent;
	
	static const size_t kMaximumTELength = 32000;
	
	
	class NotEnoughSpaceForTEKey   {};
	class NotEnoughSpaceForTEPaste {};
	
	
	static inline
	short
	min( short a, short b )
	{
		return b < a ? b : a;
	}
	
	static inline
	short
	max( short a, short b )
	{
		return a > b ? a : b;
	}
	
	void Preflight_TEKey( char c, TEHandle hTE )
	{
		if ( c != kBackspaceCharCode  &&  hTE[0]->teLength >= kMaximumTELength )
		{
			throw NotEnoughSpaceForTEKey();
		}
	}
	
	void Preflight_TEPaste( TEHandle hTE )
	{
		if ( hTE[0]->teLength + TEGetScrapLength() > kMaximumTELength )
		{
			throw NotEnoughSpaceForTEPaste();
		}
	}
	
	
	void TextEdit::Install( const Rect& bounds )
	{
		ASSERT( itsTE == NULL );
		
		itsTE = N::TENew( bounds );
		
		if ( has_TEAutoView() )
		{
			TEAutoView( true, itsTE );  // enable auto-scrolling
		}
		
		mac::app::install_custom_TEClickLoop( itsTE );
	}
	
	void TextEdit::Uninstall()
	{
		itsTE.reset();
	}
	
	void TextEdit::Insert_Key( char c )
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		Preflight_TEKey( c, hTE );
		
		::TEKey( c, hTE );
	}
	
	void TextEdit::Delete()
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		::TEDelete( hTE );
	}
	
	void TextEdit::Cut()
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		Clipboard::TECut( hTE );
	}
	
	void TextEdit::Paste()
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		Preflight_TEPaste( hTE );
		
		::TEPaste( hTE );
	}
	
	static bool LeftOrRightArrowsKeyed()
	{
		N::GetKeys_Result keys = N::GetKeys();
		
		const UInt8 high7x = keys.keyMapByteArray[ 0x7b >> 3 ];
		
		const UInt8 leftRightArrowBits = 1 << (0x7b & 0x07) | 1 << (0x7c & 0x07);
		
		return high7x & leftRightArrowBits;
	}
	
	void TextEdit::Idle( const EventRecord& )
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		if ( !hTE[0]->active )
		{
			return;
		}
		
		TEIdle( hTE );
		
		AdjustSleepForTimer( ::GetCaretTime() );
		
		if ( !LeftOrRightArrowsKeyed() )
		{
			ResetArrowKeyChordability();
		}
	}
	
	bool TextEdit::SetCursor( const EventRecord& event )
	{
		N::SetCursor( N::GetCursor( N::iBeamCursor ) );
		
		return true;
	}
	
	bool TextEdit::MouseDown( const EventRecord& event )
	{
		if ( Get_Focus() != NULL )
		{
			Change_Focus( this );
		}
		
		ASSERT( Get() != NULL );
		
		using mac::app::TEClick_contents_scope;
		
		TEClick_contents_scope scope( its_TEClickLoop_callback, this );
		
		TEClick( N::GlobalToLocal( event.where ),
		         (event.modifiers & shiftKey) != 0,
		         Get() );
		
		gExtendingSelection = false;
		
		On_UserSelect();
		
		return true;
	}
	
	
	static inline bool NewContentReplacesSelection()
	{
		// True in Mac HIG, false in The Humane Interface
		return false;
	}
	
	static inline
	bool KeyIsAllowedAgainstSelection( unsigned char c, TEHandle aTE )
	{
		if ( NewContentReplacesSelection() )
		{
			return true;
		}
		
		// Allow control keys always (backspace, arrows)
		// Allow content keys only when selection is empty (insertion point)
		return c < 0x20  ||  aTE[0]->selStart == aTE[0]->selEnd;
	}
	
	static inline bool char_is_horizontal_arrow( char c )
	{
		return (c & 0xFE) == 0x1C;
	}

	static inline bool char_is_vertical_arrow( char c )
	{
		return (c & 0xFE) == 0x1E;
	}
	
	static inline bool char_is_forward_arrow( char c )
	{
		return (c & 0xFD) == 0x1D;
	}
	
	static inline bool char_is_arrow( char c )
	{
		return (c & 0xFC) == 0x1C;
	}
	
	static inline bool char_is_delete( char c )
	{
		return c == kBackspaceCharCode  ||  c == kDeleteCharCode;
	}
	
	static bool char_is_word_char( char c )
	{
		if ( c == '_' )  return true;
		
		if ( c >= '0'  &&  c <= '9' )  return true;
		
		c |= 'a' - 'A';
		
		return c >= 'a'  &&  c <= 'z';
	}
	
	void TextEdit::Apply_Modified_Arrow( char c, bool cmdKeyIsDown, bool optionKeyIsDown )
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		const bool secret = IsSecret();
		
		// Dereferencing hTE
		const TERec& te = **hTE;
		
		if ( secret )
		{
			gSelectionExtent = char_is_forward_arrow( c ) ? te.teLength : 0;
			
			return;
		}
		
		// Dereferencing te.hText
		const char* begin = te.hText[0];
		const char* end   = begin + te.teLength;
		
		const char* p = begin + gSelectionExtent;
		
		if ( cmdKeyIsDown )
		{
			switch ( c )
			{
				case kLeftArrowCharCode:
					while ( p > begin  &&  p[-1] != '\r' )  --p;
					break;
				
				case kRightArrowCharCode:
					while ( p < end  &&  p[0] != '\r' )  ++p;
					break;
				
				case kUpArrowCharCode:
					p = begin;
					break;
				
				case kDownArrowCharCode:
					p = end;
					break;
				
			}
		}
		else if ( optionKeyIsDown )
		{
			switch ( c )
			{
				case kLeftArrowCharCode:
					if ( p > begin )
						while ( --p > begin  &&  char_is_word_char( p[-1] ) )  continue;
					break;
				
				case kRightArrowCharCode:
					if ( p < end )
						while ( ++p < end  &&  char_is_word_char( p[0] ) )  continue;
					break;
				
				case kUpArrowCharCode:
				case kDownArrowCharCode:
					mac::sys::beep();  // May move memory
					break;
			}
		}
		
		gSelectionExtent = p - begin;
	}
	
	static inline void OutOfBounds()
	{
		// left/up arrow or backspace at start, or
		// right/down arrow or forward delete at end
		
		// do nothing
	}
	
	void TextEdit::Apply_Key( const EventRecord& event )
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		const UInt32 kEitherShiftKey   = shiftKey   | rightShiftKey;
		const UInt32 kEitherOptionKey  = optionKey  | rightOptionKey;
		const UInt32 kEitherControlKey = controlKey | rightControlKey;
		
		char c =  event.message;
		
		bool cmdKeyIsDown    = event.modifiers & cmdKey;
		bool shiftKeyIsDown  = event.modifiers & kEitherShiftKey;
		bool optionKeyIsDown = event.modifiers & kEitherOptionKey;
		
		// Dereferencing hTE
		TERec& te = **hTE;
		
		short selStart = te.selStart;
		short selEnd   = te.selEnd;
		
		const bool emptySelection = selStart == selEnd;
		
		const bool forward = char_is_forward_arrow( c );
		
		const bool deleting = char_is_delete( c );
		
		if ( deleting )
		{
			// Shift-Backspace is Forward Delete
			if ( shiftKeyIsDown )
			{
				c = kDeleteCharCode;
				
				shiftKeyIsDown = false;
			}
			
			if ( (cmdKeyIsDown || optionKeyIsDown) && emptySelection )
			{
				// For Option- or Cmd-Delete, pretend we're selecting first
				c = c == kBackspaceCharCode ? kLeftArrowCharCode
											: kRightArrowCharCode;
				
				shiftKeyIsDown = true;
			}
			else if ( !emptySelection )
			{
				// On a selection, Cmd- and Option-Delete are the same as Delete.
				cmdKeyIsDown    = false;
				optionKeyIsDown = false;
				
				// On a selection, Forward Delete is the same as Backspace.
				c = kBackspaceCharCode;
			}
			else if ( c == kDeleteCharCode )
			{
				if ( selEnd == te.teLength )
				{
					OutOfBounds();  // Forward Delete at end
					
					return;
				}
				
				te.selStart =
				te.selEnd += 1;
				
				c = kBackspaceCharCode;
			}
			else if ( selStart == 0 )
			{
				OutOfBounds();  // Backspace at start
				
				return;
			}
		}
		else if ( emptySelection  &&  char_is_arrow( c )  &&  selStart == (forward ? te.teLength : 0) )
		{
			OutOfBounds();
			
			return;
		}
		
		const bool initializingSelection = char_is_arrow( c )  &&  (!gExtendingSelection || !shiftKeyIsDown);
		
		if ( initializingSelection )
		{
			// The selection anchor and extent must be initialized when we start or
			// finish extending the selection.
			gSelectionAnchor =  forward ? selStart : selEnd;
			gSelectionExtent = !forward ? selStart : selEnd;
		}
		
		gExtendingSelection = char_is_arrow( c ) && shiftKeyIsDown;
		
		if ( gExtendingSelection || char_is_vertical_arrow( c ) )
		{
			// Up/down arrow key or arrow key with Shift
			
			// Set our extent as the insertion point so we can use TextEdit's arrow
			// behavior.
			::TESetSelect( gSelectionExtent, gSelectionExtent, hTE );
		}
		
		if ( char_is_arrow( c )  &&  (cmdKeyIsDown || optionKeyIsDown) )
		{
			// Arrow key with Command or Option (and possibly Shift)
			// Delete key with Command or Option on empty selection
			
			Apply_Modified_Arrow( c, cmdKeyIsDown, optionKeyIsDown );
			
			if ( !gExtendingSelection )
			{
				// Update the real insertion point
				::TESetSelect( gSelectionExtent, gSelectionExtent, hTE );
			}
		}
		else if ( !gExtendingSelection  &&  char_is_horizontal_arrow( c )  &&  !emptySelection )
		{
			// Unmodified left/right arrow on non-empty selection
			
			// Workaround TextEdit's bug where left- or right-arrow places the
			// insertion point past the selection instead of at the edge of it.
			
			if ( c == kRightArrowCharCode )
			{
				selStart = selEnd;
			}
			else
			{
				selEnd = selStart;
			}
			
			// Update the real insertion point
			::TESetSelect( selStart, selEnd, hTE );
		}
		else if ( char_is_arrow( c ) )
		{
			// Up/down arrow key, possibly with Shift
			// Left/right arrow key with Shift or on empty selection
			
			::TEKey( c, hTE );
			
			// Update the extent
			gSelectionExtent = hTE[0]->selStart;
		}
		else if ( deleting )
		{
			// Delete key unmodified or on non-empty selection
			
			if ( selStart == selEnd )
			{
				Insert_Key( kBackspaceCharCode );
			}
			else
			{
				Delete();
			}
		}
		else
		{
			// Not an arrow or delete key
			
			Insert_Key( c );
		}
		
		if ( gExtendingSelection )
		{
			// Arrow key with Shift
			// Delete key with Command or Option on empty selection
			
			selStart = min( gSelectionAnchor, gSelectionExtent );
			selEnd   = max( gSelectionAnchor, gSelectionExtent );
			
			// Update the real selection
			::TESetSelect( selStart, selEnd, hTE );
			
			if ( deleting )
			{
				Delete();
			}
		}
	}
	
	bool TextEdit::Preprocess_Key( const EventRecord& event )
	{
		if ( IsSecret() )
		{
			return event.what == autoKey;  // eat auto-keys for password entry
		}
		
		return    Try_IgnoreAutoKey( event )
		       || Try_RepeatSearch( *this, event )
		       || (event.what == keyDown && Try_ArrowKeyChord( *this, event.message ));
	}
	
	bool TextEdit::Process_Key( const EventRecord& event )
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		const char c =  event.message;
		
		if ( !KeyIsAllowedAgainstSelection( c, hTE ) )
		{
			static UInt32 lastBeep = 0;
			
			if ( event.when - lastBeep > 45 )
			{
				lastBeep = event.when;
				
				mac::sys::beep();
			}
			
			// do nothing
		}
		else
		{
			Apply_Key( event );
			
			On_UserEdit();
		}
		
		return true;
	}
	
	bool TextEdit::KeyDown( const EventRecord& event )
	{
		ObscureCursor();
		
		const unsigned char c =  event.message;
		
		if ( Preprocess_Key( event ) )
		{
			// already handled
		}
		else if ( c <= 0x1b  &&  c != 0x08  &&  (c != 0x0d || IsSingular()) )
		{
			// Don't process control chars except arrow keys, BS, and maybe CR
			return false;
		}
		else if ( Process_Key( event ) )
		{
			Postprocess_Key( event );
		}
		else
		{
			return false;
		}
		
		return true;
	}
	
	TextEdit*
	//
	TextEdit::EnterShiftSpaceQuasimode( const EventRecord& event )
	{
		if ( IsSecret() )
		{
			return View::EnterShiftSpaceQuasimode( event );
		}
		
		return this;
	}
	
	
	static bool IsFocused( const View* that )
	{
		const View* focus = Get_Focus();
		
		return focus == NULL  ||  focus == that;
	}
	
	void TextEdit::Activate( bool activating )
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		if ( activating  &&  IsFocused( this ) )
		{
			TEActivate( hTE );
		}
		else if ( !activating  &&  hTE[0]->active )
		{
			TEDeactivate( hTE );
		}
		
		gExtendingSelection = false;
		
		ResetArrowKeyChordability();
		
		SetActive( activating );
	}
	
	void TextEdit::Focus()
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		TEActivate( hTE );
	}
	
	void TextEdit::Blur()
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		TEDeactivate( hTE );
	}
	
	void TextEdit::Cue()
	{
		Select( 0, 32767 );
	}
	
	View* TextEdit::AdvanceFocus( View* current, bool backward )
	{
		View* result = current == this ? NULL
		             : current == NULL ? this
		             :                   current;
		
		return result;
	}
	
	static void EraseBlankArea( TEHandle hTE )
	{
		TERec& te = **hTE;
		
		Rect viewRect = te.viewRect;
		
		// View height in pixels
		short viewHeight = viewRect.bottom - viewRect.top;
		
		// Text height in pixels
		// NOTE:  nLines counts displayable lines only, ignoring a trailing CR.
		int textHeight = te.nLines * te.lineHeight;
		
		int textBottom = te.destRect.top + textHeight;
		
		// If the bottom of the text doesn't reach the bottom of the viewing area,
		if ( textBottom < viewRect.bottom )
		{
			// then below the text is a blank space, which TEUpdate() ignores.
			// So we need to erase it ourselves.
			viewRect.top = textBottom;
			
			N::EraseRect( viewRect );
		}
	}
	
	void TextEdit::Draw( const Rect& bounds, bool erasing )
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		TERec& te = **hTE;
		
		if ( nucleus::operator!=( bounds, te.viewRect ) )
		{
			short dv = te.viewRect.top - te.destRect.top;
			
			te.viewRect = bounds;
			te.destRect = bounds;
			
			te.destRect.top    -= dv;
			te.destRect.bottom -= dv;
			
			TECalText( hTE );
		}
		
		EraseBlankArea( hTE );
		
		TEUpdate( &bounds, hTE );
	}
	
	bool TextEdit::UserCommand( CommandCode code )
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		switch ( code )
		{
			// Edit
			case 'undo':
				//Undo();
				break;
			
			case 'cut ':
				if ( IsSecret() )
				{
					mac::sys::beep();
					
					break;
				}
				
				Cut();
				
				On_UserEdit();
				
				break;
			
			case 'copy':
				if ( IsSecret() )
				{
					mac::sys::beep();
					
					break;
				}
				
				Clipboard::TECopy( hTE );
				break;
			
			case 'past':  // kHICommandPaste
			case 'pste':
				// Update the TE scrap just-in-time
				if ( OSErr err = TEFromScrap() )
				{
					mac::sys::beep();
					break;
				}
				
				Paste();
				
				On_UserEdit();
				
				break;
			
			case 'clea':
				Delete();
				
				On_UserEdit();
				
				break;
			
			case 'sall':  // kHICommandSelectAll
			case 'slct':
				Select( 0, 32767 );
				break;
			
			default:
				return false;
				break;
		}
		
		return true;
	}
	
	
	void TextEdit::BeginQuasimode()
	{
		N::FrameRect( mac::qd::get_portRect( mac::qd::thePort() ) );
	}
	
	void TextEdit::EndQuasimode()
	{
		N::InvalRect( mac::qd::get_portRect( mac::qd::thePort() ) );
	}
	
	TextSelection TextEdit::GetCurrentSelection() const
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		struct TextSelection result;
		
		const TERec& te = **hTE;
		
		result.start = te.selStart;
		result.end   = te.selEnd;
		
		return result;
	}
	
	TextSelection TextEdit::GetPriorSelection() const
	{
		return itsSelectionPriorToSearch;
	}
	
	void TextEdit::SetPriorSelection( const TextSelection& selection )
	{
		itsSelectionPriorToSearch = selection;
	}
	
	void TextEdit::Select( unsigned start, unsigned end )
	{
		ASSERT( Get() != NULL );
		
		TESetSelect( start, end, Get() );
		
		On_UserSelect();
	}
	
	int TextEdit::Search( const char*           pattern,
	                      std::size_t           pattern_length,
	                      const TextSelection&  selection,
	                      bool                  searchBackwards,
	                      bool                  matchAtPosition ) const
	{
		TEHandle hTE = Get();
		
		ASSERT( hTE != NULL );
		
		const TERec& te = **hTE;
		
		return TextSearch( *te.hText,
		                   te.teLength,
		                   pattern,
		                   pattern_length,
		                   selection,
		                   searchBackwards,
		                   matchAtPosition );
	}
	
	
	static short CountLinesForEditing( const TERec& te )
	{
		short teLength = te.teLength;
		
		// An empty buffer counts as one line
		if ( teLength == 0 )
		{
			return 1;
		}
		
		short    nLines = te.nLines;
		::Handle hText  = te.hText;
		
		// Find the last character in the buffer
		char c = ( *hText )[ teLength - 1 ];
		
		// If the last char is a carriage return, add an extra line
		if ( c == '\r' )
		{
			++nLines;
		}
		
		return nLines;
	}
	
	unsigned GetTextEditingHeight( const TERec& te )
	{
		return CountLinesForEditing( te ) * te.lineHeight;
	}
	
}
