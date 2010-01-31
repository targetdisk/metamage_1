/*	=============
 *	rsrc-patch.cc
 *	=============
 */

// Standard C++
#include <functional>
#include <vector>

// Standard C
#include <string.h>

// iota
#include "iota/decimal.hh"
#include "iota/hexidecimal.hh"
#include "iota/quad.hh"

// Nitrogen
#include "Nitrogen/MacErrors.h"
#include "Nitrogen/OSStatus.h"
#include "Nitrogen/Resources.h"
#include "Nitrogen/Str.h"

// Divergence
#include "Divergence/Utilities.hh"

// Orion
#include "Orion/get_options.hh"
#include "Orion/Main.hh"


namespace tool
{
	
	namespace N = Nitrogen;
	namespace NN = Nucleus;
	namespace Div = Divergence;
	namespace o = orion;
	
	
	static const char* gResFilePathname = NULL;
	
	static NN::Owned< N::ResFileRefNum > gResFile;
	
	static const char* gResType = NULL;
	static const char* gResID   = NULL;
	
	static N::Handle gHandle;
	
	static std::size_t gOffset = 0;
	
	
	// E.g. "666f6f20626171" -> "foo bar"
	static std::string decoded_hex( const std::string& hex_codes )
	{
		std::string result;
		
		// FIXME:  Verify the hex data.
		
		result.resize( hex_codes.size() / 2 );
		
		for ( std::size_t i = 0;  i < result.size();  ++i )
		{
			const char high = hex_codes[ i * 2     ];
			const char low  = hex_codes[ i * 2 + 1 ];
			
			result[ i ] = iota::decoded_hex_digit( high ) << 4
			            | iota::decoded_hex_digit( low  );
		}
		
		return result;
	}
	
	
	static void FileOptor( const char* param )
	{
		gResFilePathname = param;
		
		gResFile.Reset();  // next line resets only if resolve and open succeed
		
		gResFile = N::FSpOpenResFile( Div::ResolvePathToFSSpec( param ), N::fsRdWrPerm );
	}
	
	static void LoadOptor( const char* )
	{
		gOffset = 0;
		
		if ( gHandle )
		{
			N::ReleaseResource( gHandle );
		}
		
		if ( !gResFile.Get() )
		{
			throw N::ResFNotFound();
		}
		
		if ( gResType == NULL  ||  gResID == NULL )
		{
			throw N::ResNotFound();
		}
		
		if ( strlen( gResType ) != sizeof (::ResType) )
		{
			throw N::ParamErr();
		}
		
		N::ResType resType = N::ResType( iota::decode_quad( gResType ) );
		
		N::ResID   resID   = N::ResID( iota::parse_decimal( gResID ) );
		
		gHandle = N::Get1Resource( resType, resID );
	}
	
	static void Find( const std::string& pattern )
	{
		const char* begin = *gHandle.Get();
		const char* end   = begin + N::GetHandleSize( gHandle );
		
		const char* mark = std::search( begin + gOffset,
		                                end,
		                                pattern.begin(),
		                                pattern.end() );
		
		if ( mark == end )
		{
			throw N::ParamErr();
		}
		
		gOffset = mark - begin;
	}
	
	static void FindOptor( const char* param )
	{
		Find( param );
	}
	
	static void FindHexOptor( const char* param )
	{
		Find( decoded_hex( param ) );
	}
	
	static void SeekOptor( const char* param )
	{
		std::size_t delta = iota::parse_unsigned_decimal( param );
		
		gOffset += delta;
	}
	
	static void Write( const std::string& bytes )
	{
		if ( gOffset + bytes.size() > N::GetHandleSize( gHandle ) )
		{
			throw N::ParamErr();
		}
		
		char* begin = *gHandle.Get();
		
		std::copy( bytes.begin(), bytes.end(), begin + gOffset );
		
		gOffset += bytes.size();
		
		N::ChangedResource( gHandle );
	}
	
	static void WriteOptor( const char* param )
	{
		Write( param );
	}
	
	static void WriteHexOptor( const char* param )
	{
		Write( decoded_hex( param ) );
	}
	
	int Main( int argc, iota::argv_t argv )
	{
		NN::RegisterExceptionConversion< NN::Exception, N::OSStatus >();
		
		o::bind_option_to_variable( "--type", gResType );
		o::bind_option_to_variable( "--id",   gResID   );
		
		o::bind_option_trigger( "--file", std::ptr_fun( FileOptor ) );
		o::bind_option_trigger( "--load", std::ptr_fun( LoadOptor ) );
		o::bind_option_trigger( "--find", std::ptr_fun( FindOptor ) );
		o::bind_option_trigger( "--seek", std::ptr_fun( SeekOptor ) );
		
		o::bind_option_trigger( "--write", std::ptr_fun( WriteOptor ) );
		
		o::bind_option_trigger( "--find-hex",  std::ptr_fun( FindHexOptor  ) );
		o::bind_option_trigger( "--write-hex", std::ptr_fun( WriteHexOptor ) );
		
		//o::alias_option( "--type", "-t" );
		
		o::get_options( argc, argv );
		
		return EXIT_SUCCESS;
	}

}

