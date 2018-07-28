/*
	Dialogs.cc
	----------
*/

#include "Dialogs.hh"

// Mac OS
#ifndef __CONTROLDEFINITIONS__
#include <ControlDefinitions.h>
#endif
#ifndef __DIALOGS__
#include <Dialogs.h>
#endif
#ifndef __RESOURCES__
#include <Resources.h>
#endif
#ifndef __SOUND__
#include <Sound.h>
#endif

// Standard C
#include <string.h>

// ams-common
#include "QDGlobals.hh"


static short ANumber;
static short ACount = -1;

StringHandle DAStrings[ 4 ] : 0x0AA0;


static
Handle NewHandleOrBust( Size size )
{
	Handle h = NewHandle( size );
	
	if ( h == NULL )
	{
		SysError( 25 );
	}
	
	return h;
}

static
void set_param( short i, const unsigned char* text )
{
	StringHandle& h = DAStrings[ i ];
	
	if ( text == NULL  ||  text[ 0 ] == 0 )
	{
		if ( h != NULL )
		{
			**h = 0;
		}
		
		return;
	}
	
	if ( h == NULL )
	{
		h = (StringHandle) NewHandleOrBust( 256 );
	}
	
	memcpy( *h, text, 1 + text[ 0 ] );
}

struct expansion_storage
{
	Handle text;
	UInt16 size;  // Worst case is 127 * 255 + 1 == 32386
};

static
const expansion_storage& expand_param_text( const unsigned char* text )
{
	static expansion_storage expanded = { NewHandleOrBust( 0 ) };
	
	static UInt16 capacity = 0;
	
	UInt16 expanded_size = 0;
	
	const unsigned char* begin = text + 1;
	const unsigned char* end   = begin + text[ 0 ];
	
	for ( const unsigned char* it = begin;  it < end; )
	{
		unsigned char c = *it++;
		
		if ( c != '^'  ||  it == end )
		{
			++expanded_size;
			continue;
		}
		
		c = *it++ - '0';
		
		if ( c > 3 )
		{
			expanded_size += 2;
			continue;
		}
		
		if ( StringHandle h = DAStrings[ c ] )
		{
			expanded_size += **h;
		}
	}
	
	expanded.size = expanded_size;
	
	if ( capacity < expanded_size )
	{
		capacity = expanded_size;
		
		DisposeHandle( expanded.text );
		
		expanded.text = NewHandleOrBust( capacity );
	}
	
	char* p = *expanded.text;
	
	for ( const unsigned char* q = begin;  q < end; )
	{
		unsigned char c = *q++;
		
		if ( c != '^'  ||  q == end )
		{
			*p++ = c;
			continue;
		}
		
		c = *q++ - '0';
		
		if ( c > 3 )
		{
			*p++ = '^';
			*p++ = c + '0';
			continue;
		}
		
		if ( StringHandle h = DAStrings[ c ] )
		{
			memcpy( p, *h + 1, **h );
			
			p += **h;
		}
	}
	
	return expanded;
}

struct DialogItem
{
	Handle  handle;
	Rect    bounds;
	UInt8   type;
	UInt8   length;
};

struct DialogItemList_header
{
	short       count_minus_1;
	DialogItem  first;
};

static inline
short& dialog_item_count_minus_one( Handle items )
{
	return ((DialogItemList_header*) *items)->count_minus_1;
}

static inline
short dialog_item_count( Handle items )
{
	return dialog_item_count_minus_one( items ) + 1;
}

static inline
DialogItem* first_dialog_item( Handle items )
{
	return &((DialogItemList_header*) *items)->first;
}

static
ResID& item_ResID( DialogItem* di )
{
	return *(ResID*) (di + 1);
}

static
ResID item_ResID( const DialogItem* di )
{
	return *(const ResID*) (di + 1);
}

static
DialogItem* next( DialogItem* di )
{
	return (DialogItem*) ((char*) (di + 1) + di->length + (di->length & 1));
}

static
const DialogItem* next( const DialogItem* di )
{
	return next( const_cast< DialogItem* >( di ) );
}


#pragma mark -
#pragma mark Initialization
#pragma mark -

pascal void InitDialogs_patch( void* proc )
{
}

#pragma mark -
#pragma mark Creating and Disposing of Dialogs
#pragma mark -

pascal DialogRef NewDialog_patch( void*                 storage,
                                  const Rect*           bounds,
                                  const unsigned char*  title,
                                  Boolean               visible,
                                  short                 procID,
                                  WindowRef             behind,
                                  Boolean               closable,
                                  long                  refCon,
                                  Handle                items )
{
	void* original_storage = storage;
	
	if ( storage == NULL )
	{
		storage = NewPtr( sizeof (DialogRecord) );
		
		if ( storage == NULL )
		{
			return NULL;
		}
	}
	
	memset( storage, '\0', sizeof (DialogRecord) );
	
	WindowRef window = NewWindow( storage,
	                              bounds,
	                              title,
	                              visible,
	                              procID,
	                              behind,
	                              closable,
	                              refCon );
	
	if ( window == NULL )
	{
		if ( original_storage == NULL )
		{
			DisposePtr( (Ptr) storage );
		}
		
		return NULL;
	}
	
	WindowPeek w = (WindowPeek) window;
	DialogPeek d = (DialogPeek) window;
	
	w->windowKind = dialogKind;
	
	TextFont( systemFont );
	
	d->aDefItem = 1;
	
	d->items = items;
	
	short n_items_1 = dialog_item_count_minus_one( items );
	
	DialogItem* item = first_dialog_item( items );
	
	do
	{
		switch ( item->type & 0x7F )
		{
			case ctrlItem + btnCtrl:
				item->handle = (Handle) NewControl( window,
				                                    &item->bounds,
				                                    &item->length,
				                                    visible,
				                                    0,  // value
				                                    0,  // min
				                                    1,  // max
				                                    pushButProc,
				                                    0 );
				
				ValidRect( &item->bounds );
				break;
			
			case iconItem:
				item->handle = GetResource( 'ICON', item_ResID( item ) );
				break;
			
			default:
				break;
		}
		
		item = next( item );
	}
	while ( --n_items_1 >= 0 );
	
	return window;
}

pascal DialogRef GetNewDialog_patch( short id, void* storage, WindowRef behind )
{
	Handle h = GetResource( 'DLOG', id );
	
	if ( h == NULL )
	{
		return NULL;
	}
	
	const DialogTemplate dlog = *(const DialogTemplate*) *h;
	
	ReleaseResource( h );
	
	h = GetResource( 'DITL', dlog.itemsID );
	
	if ( h == NULL )
	{
		return NULL;
	}
	
	DetachResource( h );
	
	DialogRef dialog = NewDialog( storage,
	                              &dlog.boundsRect,
	                              dlog.title,
	                              dlog.visible,
	                              dlog.procID,
	                              behind,
	                              dlog.goAwayFlag,
	                              dlog.refCon,
	                              h );
	
	return dialog;
}

pascal void CloseDialog_patch( DialogRef dialog )
{
	DialogPeek d = (DialogPeek) dialog;
	
	short n_items_1 = dialog_item_count_minus_one( d->items );
	
	const DialogItem* item = first_dialog_item( d->items );
	
	do
	{
		if ( Handle h = item->handle )
		{
			switch ( item->type & 0x7F )
			{
				case iconItem:
					ReleaseResource( h );
					break;
				
				default:
					break;
			}
		}
		
		item = next( item );
	}
	while ( --n_items_1 >= 0 );
	
	DisposeHandle( d->items );
	
	CloseWindow( dialog );
}

pascal void DisposeDialog_patch( DialogRef dialog )
{
	CloseDialog( dialog );
	
	DisposePtr( (Ptr) dialog );
}

#pragma mark -
#pragma mark Handling Dialog Events
#pragma mark -

static
bool invoke_defItem( DialogPeek d )
{
	const DialogItem* item = first_dialog_item( d->items );
	
	if ( d->aDefItem > 1 )
	{
		item = next( item );
	}
	
	const UInt8 type = item->type;
	
	if ( type == ctrlItem + btnCtrl )
	{
		UInt32 dummy;
		
		ControlRef button = (ControlRef) item->handle;
		
		HiliteControl( button, kControlButtonPart );
		Delay( 8, &dummy );
		HiliteControl( button, kControlNoPart );
	}
	
	return ! (type & 0x80);
}

pascal void ModalDialog_patch( ModalFilterUPP filterProc, short* itemHit )
{
	QDGlobals& qd = get_QDGlobals();
	
	WindowRef window = qd.thePort;
	
	WindowPeek w = (WindowPeek) window;
	DialogPeek d = (DialogPeek) window;
	
	const long sleep = 0x7fffffff;
	
	while ( true )
	{
		EventRecord event;
		
		if ( WaitNextEvent( everyEvent, &event, sleep, NULL ) )
		{
			switch ( event.what )
			{
				case updateEvt:
					BeginUpdate( window );
					DrawDialog( window );
					EndUpdate( window );
					break;
				
				case mouseDown:
				{
					if ( ! PtInRgn( event.where, w->contRgn ) )
					{
						SysBeep( 30 );
						continue;
					}
					
					Point pt = event.where;
					GlobalToLocal( &pt );
					
					// item count minus one
					short n_items_1 = dialog_item_count_minus_one( d->items );
					
					const DialogItem* item = first_dialog_item( d->items );
					
					short item_index = 0;
					
					while ( item_index++ <= n_items_1 )
					{
						const UInt8 type = item->type;
						
						if ( PtInRect( pt, &item->bounds ) )
						{
							if ( (type & 0x7c) == ctrlItem )
							{
								ControlRef control = (ControlRef) item->handle;
								
								if ( ! TrackControl( control, pt, NULL ) )
								{
									break;
								}
							}
							
							if ( ! (type & 0x80) )
							{
								*itemHit = item_index;
								return;
							}
						}
						
						item = next( item );
					}
					
					break;
				}
				
				case keyDown:
				{
					switch ( (char) event.message )
					{
						case kEnterCharCode:
						case kReturnCharCode:
							if ( invoke_defItem( d ) )
							{
								*itemHit = d->aDefItem;
								return;
							}
						
						default:
							break;
					}
					
					break;
				}
				
				default:
					break;
			}
		}
	}
}

pascal void DrawDialog_patch( DialogRef dialog )
{
	DrawControls( dialog );
	
	DialogPeek d = (DialogPeek) dialog;
	
	short n_items_1 = dialog_item_count_minus_one( d->items );
	
	const DialogItem* item = first_dialog_item( d->items );
	
	short item_index = 1;
	
	do
	{
		const Rect& bounds = item->bounds;
		
		switch ( item->type & 0x7F )
		{
			case userItem:
				if ( UserItemUPP userProc = (UserItemUPP) item->handle )
				{
					userProc( dialog, item_index );
				}
				break;
			
			case editText:
			{
				Rect box = bounds;
				
				InsetRect( &box, -3, -3 );
				FrameRect( &box );
			}
			// fall through
			
			case statText:
			{
				const UInt8* text = &item->length;
				
				const expansion_storage& expansion = expand_param_text( text );
				
				TETextBox( *expansion.text, expansion.size, &bounds, 0 );
				break;
			}
			
			case iconItem:
				if ( Handle h = item->handle )
				{
					PlotIcon( &bounds, h );
				}
				else
				{
					FrameRect( &bounds );
				}
				break;
			
			case picItem:
				if ( PicHandle picture = (PicHandle) item->handle )
				{
				}
				else
				{
					QDGlobals& qd = get_QDGlobals();
					
					FillRect( &bounds, &qd.ltGray );
					FrameRect( &bounds );
				}
				break;
			
			default:
				break;
		}
		
		item = next( item );
		
		++item_index;
	}
	while ( --n_items_1 >= 0 );
}

#pragma mark -
#pragma mark Invoking Alerts
#pragma mark -

/*
	Apple's Human Interface Guidelines state that there should be 13 vertical
	and 23 horizontal white pixels between, respectively the top and left edges
	of the window frame and the dialog/alert icon.  Since the frame itself has
	a 3-pixel margin already, that reduces the figures to 10 and 20.
*/

static const Rect alert_icon_bounds = { 10, 20, 10 + 32, 20 + 32 };

static
void DITL_append_icon( Handle items, ResID icon_id )
{
	short n_items_1 = dialog_item_count_minus_one( items );
	
	DialogItem* item = first_dialog_item( items );
	
	do
	{
		item = next( item );
	}
	while ( --n_items_1 >= 0 );
	
	const short added_length = sizeof (DialogItem)
	                         + sizeof (ResID);  // 16 bytes
	
	const Size old_size = (char*) item - *items;
	
	SetHandleSize( items, old_size + added_length );
	
	item = (DialogItem*) (*items + old_size);
	
	item->handle = NULL;
	item->bounds = alert_icon_bounds;
	item->type   = iconItem + itemDisable;
	item->length = sizeof (ResID);
	
	item_ResID( item ) = icon_id;
	
	++dialog_item_count_minus_one( items );
}

static
void DITL_append_userItem( Handle items, UserItemUPP proc, const Rect& bounds )
{
	short n_items_1 = dialog_item_count_minus_one( items );
	
	DialogItem* item = first_dialog_item( items );
	
	do
	{
		item = next( item );
	}
	while ( --n_items_1 >= 0 );
	
	const short added_length = sizeof (DialogItem);  // 14 bytes
	
	const Size old_size = (char*) item - *items;
	
	SetHandleSize( items, old_size + added_length );
	
	item = (DialogItem*) (*items + old_size);
	
	item->handle = (Handle) proc;
	item->bounds = bounds;
	item->type   = userItem + itemDisable;
	item->length = 0;
	
	++dialog_item_count_minus_one( items );
}

static
pascal void default_button_outline( DialogRef dialog, short index )
{
	DialogPeek d = (DialogPeek) dialog;
	
	const DialogItem* item = first_dialog_item( d->items );
	
	if ( d->aDefItem > 1 )
	{
		item = next( item );
	}
	
	if ( item->type == ctrlItem + btnCtrl )
	{
		Rect bounds = item->bounds;
		
		InsetRect( &bounds, -4, -4 );
		
		PenSize( 3, 3 );
		FrameRoundRect( &bounds, 16, 16 );
		PenNormal();
	}
}

static
short basic_Alert( short alertID, ModalFilterUPP filterProc, ResID icon_id )
{
	Handle h = GetResource( 'ALRT', alertID );
	
	if ( h == NULL )
	{
		return ResError();
	}
	
	AlertTHndl alert = (AlertTHndl) h;
	
	const Rect bounds   = alert[0]->boundsRect;
	const short itemsID = alert[0]->itemsID;
	const short stages  = alert[0]->stages;
	
	ReleaseResource( h );
	
	h = GetResource( 'DITL', itemsID );
	
	if ( h == NULL )
	{
		return ResError();
	}
	
	DetachResource( h );
	
	if ( icon_id >= 0 )
	{
		DITL_append_icon( h, icon_id );
	}
	
	DialogRef dialog = NewDialog( NULL,
	                              &bounds,
	                              "\p",   // dBoxProc doesn't draw a title
	                              true,   // visible
	                              dBoxProc,
	                              (WindowRef) -1,
	                              false,  // not closable
	                              0,      // refCon
	                              h );
	
	if ( dialog == NULL )
	{
		return MemError();
	}
	
	DITL_append_userItem( h, &default_button_outline, dialog->portRect );
	
	if ( alertID != ANumber )
	{
		ANumber = alertID;
		ACount  = 0;
	}
	else if ( ACount < 3 )
	{
		++ACount;
	}
	
	DialogPeek d = (DialogPeek) dialog;
	
	d->aDefItem = ((stages >> 4 * ACount + 3) & 0x1) + 1;
	
	short itemHit = 0;
	
	ModalDialog( NULL, &itemHit );
	
	DisposeDialog( dialog );
	
	return itemHit;
}

pascal short Alert_patch( short alertID, ModalFilterUPP filterProc )
{
	return basic_Alert( alertID, filterProc, -1 );
}

pascal short StopAlert_patch( short alertID, ModalFilterUPP filterProc )
{
	return basic_Alert( alertID, filterProc, kStopIcon );
}

pascal short NoteAlert_patch( short alertID, ModalFilterUPP filterProc )
{
	return basic_Alert( alertID, filterProc, kNoteIcon );
}

pascal short CautionAlert_patch( short alertID, ModalFilterUPP filterProc )
{
	return basic_Alert( alertID, filterProc, kCautionIcon );
}

#pragma mark -
#pragma mark Manipulating Items in Dialogs and Alerts
#pragma mark -

pascal void ParamText_patch( const unsigned char*  p1,
                             const unsigned char*  p2,
                             const unsigned char*  p3,
                             const unsigned char*  p4 )
{
	set_param( 0, p1 );
	set_param( 1, p2 );
	set_param( 2, p3 );
	set_param( 3, p4 );
}
