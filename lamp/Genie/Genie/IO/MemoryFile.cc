/*	=============
 *	MemoryFile.cc
 *	=============
 */

#include "Genie/IO/MemoryFile.hh"

// Standard C++
#include <algorithm>

// poseven
#include "poseven/types/errno_t.hh"

// vfs
#include "vfs/filehandle.hh"
#include "vfs/memory_mapping.hh"
#include "vfs/node.hh"
#include "vfs/filehandle/methods/bstore_method_set.hh"
#include "vfs/filehandle/methods/filehandle_method_set.hh"
#include "vfs/filehandle/methods/general_method_set.hh"


namespace Genie
{
	
	namespace p7 = poseven;
	
	
	struct buffer_extra
	{
		char*   base;  // base address
		size_t  size;
	};
	
	class MemoryFileHandle : public vfs::filehandle
	{
		private:
			buffer_extra  extra;
		
		public:
			MemoryFileHandle( const vfs::node&  file,
			                  int               flags,
			                  char*             base,
			                  std::size_t       size );
			
			ssize_t Positioned_Read( char* buffer, size_t n_bytes, off_t offset );
			
			ssize_t Positioned_Write( const char* buffer, size_t n_bytes, off_t offset );
			
			off_t GetEOF()  { return extra.size; }
			
			vfs::memory_mapping_ptr Map( size_t length, int prot, int flags, off_t offset );
	};
	
	
	static ssize_t buffer_pread( vfs::filehandle* file, char* buffer, size_t n, off_t offset )
	{
		return static_cast< MemoryFileHandle& >( *file ).Positioned_Read( buffer, n, offset );
	}
	
	ssize_t MemoryFileHandle::Positioned_Read( char* buffer, size_t n_bytes, off_t offset )
	{
		if ( offset >= extra.size )
		{
			return 0;
		}
		
		n_bytes = std::min< size_t >( n_bytes, extra.size - offset );
		
		memcpy( buffer, extra.base + offset, n_bytes );
		
		return n_bytes;
	}
	
	static off_t buffer_geteof( vfs::filehandle* file )
	{
		return static_cast< MemoryFileHandle& >( *file ).GetEOF();
	}
	
	static ssize_t buffer_pwrite( vfs::filehandle* file, const char* buffer, size_t n, off_t offset )
	{
		return static_cast< MemoryFileHandle& >( *file ).Positioned_Write( buffer, n, offset );
	}
	
	ssize_t MemoryFileHandle::Positioned_Write( const char* buffer, size_t n_bytes, off_t offset )
	{
		if ( n_bytes == 0 )
		{
			return 0;
		}
		
		if ( offset >= extra.size )
		{
			p7::throw_errno( ENOSPC );
		}
		
		n_bytes = std::min< size_t >( n_bytes, extra.size - offset );
		
		memcpy( extra.base + offset, buffer, n_bytes );
		
		return n_bytes;
	}
	
	static vfs::memory_mapping_ptr buffer_mmap( vfs::filehandle* that, size_t length, int prot, int flags, off_t offset )
	{
		return static_cast< MemoryFileHandle& >( *that ).Map( length, prot, flags, offset );
	}
	
	vfs::memory_mapping_ptr
	//
	MemoryFileHandle::Map( size_t length, int prot, int flags, off_t offset )
	{
		if ( offset + length > extra.size )
		{
			p7::throw_errno( ENXIO );
		}
		
		return new vfs::memory_mapping( extra.base + offset, length, flags );
	}
	
	static const vfs::bstore_method_set buffer_bstore_methods =
	{
		&buffer_pread,
		&buffer_geteof,
		&buffer_pwrite,
	};
	
	static const vfs::general_method_set buffer_general_methods =
	{
		&buffer_mmap,
	};
	
	static const vfs::filehandle_method_set buffer_methods =
	{
		&buffer_bstore_methods,
		NULL,
		NULL,
		&buffer_general_methods,
	};
	
	
	MemoryFileHandle::MemoryFileHandle( const vfs::node&  file,
	                                    int               flags,
	                                    char*             base,
	                                    std::size_t       size )
	:
		vfs::filehandle( &file, flags, &buffer_methods )
	{
		extra.base = base;
		extra.size = size;
	}
	
	vfs::filehandle_ptr open_buffer_file( const vfs::node&  file,
	                                      int               flags,
	                                      char*             addr,
	                                      std::size_t       size )
	{
		return new MemoryFileHandle( file,
		                             flags,
		                             addr,
		                             size );
	}
	
}

