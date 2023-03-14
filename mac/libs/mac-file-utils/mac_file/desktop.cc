/*
	desktop.cc
	----------
*/

#include "mac_file/desktop.hh"

// mac-file-utils
#include "mac_file/directory.hh"


#pragma exceptions off


namespace mac  {
namespace file {

#if ! __LP64__

OSErr get_desktop_APPL( FSSpec&  result,
                        short    vRefNum,
                        OSType   signature,
                        short    index )
{
	OSErr err;
	DTPBRec pb;
	
	pb.ioVRefNum = vRefNum;
	pb.ioNamePtr = NULL;
	
	err = PBDTGetPath( &pb );
	
	if ( err == noErr )
	{
		pb.ioNamePtr     = result.name;
		pb.ioIndex       = index;
		pb.ioFileCreator = signature;
		
		err = PBDTGetAPPLSync( &pb );
		
		if ( err == noErr )
		{
			result.vRefNum = vRefNum;
			result.parID   = pb.ioAPPLParID;
		}
	}
	
	return err;
}

OSErr get_desktop_APPL( FSSpec&  result,
                        OSType   signature )
{
	OSErr err;
	short vRefNum = -1;
	
	do
	{
		err = get_desktop_APPL( result, vRefNum, signature, 0 );
		
		if ( err == noErr )
		{
			types::VRefNum_DirID parent = directory( result );
			
			if ( parent.vRefNum )
			{
				return err;  // noErr
			}
			
			err = parent.dirID;
		}
		
		--vRefNum;
	}
	while ( err != nsvErr );
	
	return afpItemNotFound;
}

long get_desktop_comment( short            vRefNum,
                          long             dirID,
                          ConstStr63Param  name,
                          char*            buffer,
                          UInt32           length )
{
	OSErr err;
	DTPBRec pb;
	
	pb.ioVRefNum = vRefNum;
	pb.ioNamePtr = NULL;
	
	err = PBDTGetPath( &pb );
	
	if ( err == noErr )
	{
		pb.ioNamePtr = const_cast< unsigned char* >( name );
		pb.ioDirID   = dirID;
		
		pb.ioDTBuffer   = buffer;
		pb.ioDTReqCount = length;
		
		err = PBDTGetCommentSync( &pb );
	}
	
	return err ? err : pb.ioDTActCount;
}

OSErr set_desktop_comment( short            vRefNum,
                           long             dirID,
                           ConstStr63Param  name,
                           const char*      buffer,
                           UInt32           length )
{
	OSErr err;
	DTPBRec pb;
	
	pb.ioVRefNum = vRefNum;
	pb.ioNamePtr = NULL;
	
	err = PBDTGetPath( &pb );
	
	if ( err == noErr )
	{
		pb.ioNamePtr = const_cast< unsigned char* >( name );
		pb.ioDirID   = dirID;
		
		pb.ioDTBuffer   = const_cast< char* >( buffer );
		pb.ioDTReqCount = length;
		
		err = PBDTSetCommentSync( &pb );
	}
	
	return err;
}

#endif  // #if ! __LP64__

}
}