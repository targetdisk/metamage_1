/*
	documents.hh
	------------
*/

#ifndef DOCUMENTS_HH
#define DOCUMENTS_HH

// Standard
#include <stdint.h>


struct FCB;
struct HFileParam;
struct VCB;

typedef short OSErr;


extern int docfs_fd;

void mount_virtual_documents_volume();

const uint8_t* documents_get_nth( VCB* vcb, short n );

const uint8_t* documents_lookup( VCB* vcb, const uint8_t* name );

OSErr documents_Close( FCB* fcb );
OSErr documents_Write( FCB* fcb, const char* buffer, uint32_t length );

OSErr documents_Create( VCB* vcb, const uint8_t* name );

OSErr documents_open_fork( short trap_word, FCB* fcb, const uint8_t* name );

OSErr documents_FlushFile( FCB* fcb );

OSErr documents_GetFileInfo( HFileParam* pb, const uint8_t* name );
OSErr documents_SetFileInfo( HFileParam* pb, const uint8_t* name );

#endif
