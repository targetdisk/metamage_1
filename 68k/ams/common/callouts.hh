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

DEFINE_CALLOUT( struct timeval*,
                timeval_from_microseconds,
                (struct timeval*     dst : __A0,
                 unsigned long long* src : __A1 ),
                0xFFDA )

DEFINE_CALLOUT( struct timeval*,
                timeval_from_nanoseconds,
                (struct timeval*     dst : __A0,
                 unsigned long long* src : __A1 ),
                0xFFD8 )

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

DEFINE_CALLOUT( void*,
                fast_mempcpy,
                (void*       dst : __A0,
                 void const* src : __A1,
                 unsigned    n   : __D0 ),
                0xFFD0 )

DEFINE_CALLOUT( int,
                fast_memcmp,
                (void const* a : __A0,
                 void const* b : __A1,
                 unsigned    n : __D0 ),
                0xFFC6 )

#undef DEFINE_CALLOUT

inline
int fast_memequ( const void* a, const void* b, unsigned long n )
{
	return fast_memcmp( a, b, n ) == 0;
}

static inline
asm bool mem_test( const void* addr : __A0, int size : __D0 )
{
	MOVEQ.L  #0,D1
	MOVEQ.L  #0,D2
	
	JSR      0xFFFFFFCE
}

inline asm
void fast_memcpy( void* : __A1, const void* : __A0, unsigned long : __D0 )
{
	0xA22E  // _BlockMoveData
}

inline asm
void fast_memmove( void* : __A1, const void* : __A0, unsigned long : __D0 )
{
	0xA22E  // _BlockMoveData
}

unsigned long CmpString_sans_case( const unsigned char*  a : __A0,
                                   unsigned char         n : __D1,
                                   const unsigned char*  b : __A1,
                                   unsigned char         m : __D2 );

inline
unsigned long EqualString_sans_case( const unsigned char*  a : __A0,
                                     unsigned char         n : __D1,
                                     const unsigned char*  b : __A1,
                                     unsigned char         m : __D2 )
{
	return ! CmpString_sans_case( a, n, b, m );
}

inline
unsigned long EqualString_sans_case( const unsigned char*  a : __A0,
                                     unsigned char         n : __D1,
                                     const unsigned char*  b : __A1 )
{
	unsigned char m = *b++;
	
	return EqualString_sans_case( a, n, b, m );
}

inline
unsigned long EqualString_sans_case( const unsigned char*  a : __A0,
                                     const unsigned char*  b : __A1 )
{
	unsigned char n = *a++;
	unsigned char m = *b++;
	
	return EqualString_sans_case( a, n, b, m );
}

long RelString_sans_case( const unsigned char*  a : __A0,
                          unsigned char         n : __D1,
                          const unsigned char*  b : __A1,
                          unsigned char         m : __D2 );

inline
long RelString_sans_case( const unsigned char*  a : __A0,
                          const unsigned char*  b : __A1 )
{
	unsigned char n = *a++;
	unsigned char m = *b++;
	
	return RelString_sans_case( a, n, b, m );
}

#endif
