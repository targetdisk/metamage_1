/*
	PoPLegacyEnabler.cc
	-------------------
	
	Prince of Persia Legacy Enabler for Mac OS
	
	Copyright 2022-2023, Joshua Juran.  All rights reserved.
	
	License:  AGPLv3+ (see bottom for legal boilerplate)
	
	This is a clean and simple workaround to prevent Prince of Persia from
	crashing in System 6 (and perhaps earlier versions) on a compact Mac.
	
	Prince of Persia installs a VBL task which it later tries to find in
	the VBL queue, but due to its flawed search code, it only succeeds
	when its VBL task is not at the queue's head.  This isn't an issue in
	System 7 or on color machines (where the system evidently installs its
	own VBL task at startup), but it is in System 6 on a Mac Plus, at least.
	
	The workaround applied here is to install a dummy VBL task which does
	nothing, but occupies the head slot which Prince of Persia ignores.
	
*/

// Mac OS
#ifndef __RESOURCES__
#include <Resources.h>
#endif
#ifndef __RETRACE__
#include <Retrace.h>
#endif


#pragma exceptions off


QHdr VBLQueue : 0x0160;

static
asm
void dummy_VBL_proc( VBLTask* task : __A0 )
{
	// Don't do anything; just stick around in the queue
	
	MOVE.W   #0x7fff,10(A0)  // task->vblCount = 0x7FFF;
	
	RTS
}

static VBLTask dummy_VBL_task;

int main()
{
	if ( VBLQueue.qHead == NULL )
	{
		Handle self = Get1Resource( 'INIT', 0 );
		
		DetachResource( self );
		
		dummy_VBL_task.qType    = vType;
		dummy_VBL_task.vblAddr  = (VBLUPP) &dummy_VBL_proc;
		dummy_VBL_task.vblCount = 0x7FFF;
		
		VInstall( (QElem*) &dummy_VBL_task );
	}
	
	return 0;
}

/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
