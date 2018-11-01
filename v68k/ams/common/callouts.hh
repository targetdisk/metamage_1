/*
	callouts.hh
	-----------
*/

#ifndef CALLOUTS_HH
#define CALLOUTS_HH

#define DEFINE_CALLOUT( result, name, args, addr )  \
inline asm result name args  { JSR  (0xFFFF0000 | addr) }

DEFINE_CALLOUT( void, module_suspend, (), 0xFFF8 )

DEFINE_CALLOUT( void, lock_screen,   (), 0xFFEC )
DEFINE_CALLOUT( void, unlock_screen, (), 0xFFEA )

DEFINE_CALLOUT( unsigned long, get_Ticks, (), 0xFFDE )

DEFINE_CALLOUT( void,
                fast_memset,
                (void* : __A0, char : __D1, unsigned long : __D0 ),
                0xFFD6 )

DEFINE_CALLOUT( void,
                fast_memnot,
                (void* : __A0, unsigned long : __D0 ),
                0xFFD4 )

DEFINE_CALLOUT( void,
                fast_rshift,
                (void*       dst : __A1,
                 void const* src : __A0,
                 unsigned    n   : __D0,  // src byte count; dst count is n + 1
                 char        x   : __D1 ),  // right shift bit count
                0xFFD2 )

#undef DEFINE_CALLOUT

#endif
