/*	========
 *	netdb.cc
 *	========
 */

// POSIX
#include "netdb.h"

// Nitrogen
#include "Nitrogen/OpenTransportProviders.h"

// Genie
#include "Genie/Process.hh"
#include "Genie/SystemCallRegistry.hh"
#include "Genie/SystemCalls.hh"
#include "Genie/Utilities/ShareOpenTransport.hh"


namespace Genie
{
	
	namespace N = Nitrogen;
	namespace NN = Nucleus;
	
	
	static pascal void YieldingNotifier( void*        context,
	                                     OTEventCode  code,
	                                     OTResult     result,
	                                     void*        cookie )
	{
		switch ( code )
		{
			case kOTSyncIdleEvent:
				try
				{
					Yield( kInterruptNever );  // FIXME
				}
				catch ( ... )
				{
				}
				
				break;
			
			case kOTProviderWillClose:
				break;
			
			case kOTProviderIsClosed:
				break;
			
			default:
				break;
		}
	}
	
	
	static NN::Owned< InetSvcRef > InternetServices()
	{
		static OTNotifyUPP gNotifyUPP = ::NewOTNotifyUPP( YieldingNotifier );
		
		NN::Owned< N::InetSvcRef > provider = N::OTOpenInternetServices( kDefaultInternetServicesPath );
		
		N::OTInstallNotifier( provider, gNotifyUPP, NULL );
		
		N::OTUseSyncIdleEvents( provider, true );
		
		return provider;
	}
	
	
	static struct hostent* gethostbyname( const char* name )
	{
		SystemCallFrame frame( "gethostbyname" );
		
		static ::InetHostInfo  info;
		static struct hostent  host_entry;
		static ::InetHost*     addr_list[10];
		
		OpenTransportShare sharedOpenTransport;
		
		try
		{
			info = N::OTInetStringToAddress( InternetServices(),
			                                 (char*) name );
		}
		catch ( ... )
		{
			return NULL;
		}
		
		addr_list[0] = &info.addrs[0];
		
		host_entry.h_name      = info.name;
		host_entry.h_aliases   = NULL;
		host_entry.h_addrtype  = AF_INET;
		host_entry.h_length    = sizeof (::InetHost);
		host_entry.h_addr_list = reinterpret_cast< char** >( addr_list );
		
		return &host_entry;
	}
	
	static OSStatus OTInetMailExchange_k( char* domain, UInt16* count, InetMailExchange* result )
	{
		SystemCallFrame frame( "OTInetMailExchange" );
		
		OpenTransportShare sharedOpenTransport;
		
		return ::OTInetMailExchange( InternetServices(), domain, count, result );
	}
	
	#pragma force_active on
	
	REGISTER_SYSTEM_CALL( gethostbyname );
	
#if !TARGET_API_MAC_CARBON
	
	REGISTER_SYSTEM_CALL( OTInetMailExchange_k );
	
#endif
	
	#pragma force_active reset
	
}

