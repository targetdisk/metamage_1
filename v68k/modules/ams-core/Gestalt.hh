/*
	Gestalt.hh
	----------
*/

#ifndef GESTALT_HH
#define GESTALT_HH

typedef short (*Gestalt_ProcPtr)( unsigned long : __D0, long* : __A1 ) : __D0;

extern Gestalt_ProcPtr old_Gestalt;

pascal short Gestalt_patch( unsigned long  selector : __D0,
                            long*          response : __A1 ) : __D0;

#endif
